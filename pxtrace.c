#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_pxtrace.h"
#include "pxtrace_arginfo.h"
#include "zend_extensions.h"

ZEND_DECLARE_MODULE_GLOBALS(pxtrace)
static PHP_MINFO_FUNCTION(pxtrace);
static PHP_MINIT_FUNCTION(pxtrace);
static PHP_MSHUTDOWN_FUNCTION(pxtrace);
static PHP_RINIT_FUNCTION(pxtrace);
static PHP_INI_MH(pxtrace_on_update_output_path);
static int pxtrace_extension_startup(zend_extension *ext);
static void pxtrace_execute_ex(zend_execute_data *frame);
ZEND_DLEXPORT void pxtrace_statement_handler(zend_execute_data *frame);
static int pxtrace_read_line(char *fpath, int lineno, char *line, size_t nline);
static int pxtrace_open_output(void);
static int pxtrace_get_current_stack_depth(void);
static void pxtrace_enable(void);
static void pxtrace_disable(void);
static void pxtrace_close_file(void);
static char *pxtrace_color(int c);

zend_module_entry pxtrace_module_entry = {
    STANDARD_MODULE_HEADER,
    "pxtrace",
    ext_functions,
    PHP_MINIT(pxtrace),
    PHP_MSHUTDOWN(pxtrace),
    PHP_RINIT(pxtrace),
    NULL,
    PHP_MINFO(pxtrace),
    PXTRACE_VERSION,
    STANDARD_MODULE_PROPERTIES
};

ZEND_DLEXPORT zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

ZEND_DLEXPORT zend_extension zend_extension_entry = {
    (char *)PXTRACE_NAME,
    (char *)PXTRACE_VERSION,
    (char *)PXTRACE_AUTHOR,
    (char *)PXTRACE_URL,
    (char *)PXTRACE_COPYRIGHT,
    pxtrace_extension_startup,
    NULL, // shutdown
    NULL, // activate
    NULL, // deactivate
    NULL, // message_handler
    NULL, // op_array_handler
    pxtrace_statement_handler,
    NULL, // fcall_begin_handler
    NULL, // fcall_end_handler
    NULL, // op_array_ctor
    NULL, // op_array_dtor
    STANDARD_ZEND_EXTENSION_PROPERTIES
};

#ifdef COMPILE_DL_PXTRACE
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(pxtrace)
#endif

PHP_INI_BEGIN()
        PHP_INI_ENTRY("pxtrace.output_path",      "@stderr", PHP_INI_ALL, pxtrace_on_update_output_path)
    STD_PHP_INI_ENTRY("pxtrace.auto_enable",      "0",       PHP_INI_ALL, OnUpdateLong, auto_enable, zend_pxtrace_globals, pxtrace_globals)
    STD_PHP_INI_ENTRY("pxtrace.trace_statements", "0",       PHP_INI_ALL, OnUpdateLong, trace_statements, zend_pxtrace_globals, pxtrace_globals)
    STD_PHP_INI_ENTRY("pxtrace.ansi_color",       "0",       PHP_INI_ALL, OnUpdateLong, ansi_color, zend_pxtrace_globals, pxtrace_globals)
PHP_INI_END()

enum PXTRACE_COLOR {
    PXTRACE_COLOR_RESET,
    PXTRACE_COLOR_TIME,
    PXTRACE_COLOR_DEPTH,
    PXTRACE_COLOR_PATH,
    PXTRACE_COLOR_FUNC,
    PXTRACE_COLOR_CODE
};

static PHP_MINFO_FUNCTION(pxtrace) {
    php_info_print_table_start();
    php_info_print_table_header(2, "pxtrace support", "enabled");
    php_info_print_table_header(2, "extension version", PXTRACE_VERSION);
    php_info_print_table_end();
}

static PHP_MINIT_FUNCTION(pxtrace) {
    REGISTER_INI_ENTRIES();
    if (PXTRACE_G(auto_enable)) {
        pxtrace_enable();
    }
    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(pxtrace) {
    pxtrace_disable();
    return SUCCESS;
}

static PHP_RINIT_FUNCTION(pxtrace) {
    if (PXTRACE_G(trace_statements)) {
        CG(compiler_options) |= ZEND_COMPILE_EXTENDED_STMT;
    }
    return SUCCESS;
}

static PHP_INI_MH(pxtrace_on_update_output_path) {
    pxtrace_close_file();
    PXTRACE_G(output_path) = ZSTR_VAL(new_value);
    return SUCCESS;
}

static int pxtrace_extension_startup(zend_extension *ext) {
    return zend_startup_module(&pxtrace_module_entry);
}

static void pxtrace_execute_ex(zend_execute_data *frame) {
    zend_function *zf = frame->func;

    char *fname = PXTRACE_G(depth) == 0 ? "<main>" : "<require>";
    if (zf->common.function_name) {
        fname = ZSTR_VAL(zf->common.function_name);
    }

    char *class_name = "";
    if (zf->common.scope) {
        class_name = ZSTR_VAL(zf->common.scope->name);
    }

    char *fpath = "<internal>";
    int lineno = -1;
    zend_execute_data *prev_data = frame->prev_execute_data;
    if (prev_data) {
        zend_function *prev_zf = prev_data->func;
        if (prev_zf && prev_zf->type == ZEND_USER_FUNCTION) {
            fpath = ZSTR_VAL(prev_zf->op_array.filename);
            lineno = prev_data->opline ? prev_data->opline->lineno : -1;
        }
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t cur_ns = (uint64_t)ts.tv_sec * (uint64_t)1000000000 + (uint64_t)ts.tv_nsec;

    if (!PXTRACE_G(output_file)) {
        if (!pxtrace_open_output()) {
            pxtrace_disable();
            return;
        }
    }

    fprintf(PXTRACE_G(output_file),
        "%s%20.3f%s "
        "%s% 4d%s "
        "%*s"
        "%s%s:%-4d%s "
        "%s%s%s%s%s\n",
        pxtrace_color(PXTRACE_COLOR_TIME), (float)(PXTRACE_G(last_ns) == 0 ? 0 : (cur_ns - PXTRACE_G(last_ns))) / 1000.f, pxtrace_color(PXTRACE_COLOR_RESET),
        pxtrace_color(PXTRACE_COLOR_DEPTH), PXTRACE_G(depth), pxtrace_color(PXTRACE_COLOR_RESET),
        (PXTRACE_G(depth) < 0 ? 0 : PXTRACE_G(depth)) * 2, "",
        pxtrace_color(PXTRACE_COLOR_PATH), fpath, lineno, pxtrace_color(PXTRACE_COLOR_RESET),
        pxtrace_color(PXTRACE_COLOR_FUNC), class_name, *class_name != '\0' ? "::" : "", fname, pxtrace_color(PXTRACE_COLOR_RESET)
    );

    PXTRACE_G(last_ns) = cur_ns;

    PXTRACE_G(depth) += 1;
    (PXTRACE_G(orig_execute_ex))(frame);
    PXTRACE_G(depth) -= 1;
}

ZEND_DLEXPORT void pxtrace_statement_handler(zend_execute_data *frame) {
    if (!PXTRACE_G(trace_statements) || !PXTRACE_G(enabled)) {
        return;
    }

    if (!frame->opline || !frame->func || frame->func->type != ZEND_USER_FUNCTION) {
        return;
    }

    char *fpath = ZSTR_VAL(frame->func->op_array.filename);
    int lineno = frame->opline->lineno;

    char line[4096];
    line[0] = '\0';
    int linelen = pxtrace_read_line(fpath, lineno, line, sizeof(line));
    char *linep = line;

    if (linelen <= 0) {
        // Failed to get line from file
        return;
    } else {
        // Trim leading space
        while (*linep && isspace(*linep)) {
            ++linep;
            --linelen;
        }

        // Printing a line like '{' is generally not helpful.
        // Make sure there is at least one isalnum char.
        char *linetmp = linep;
        int has_alnum = 0;
        while (*linetmp && !(has_alnum = isalnum(*linetmp))) {
            ++linetmp;
        }
        if (!has_alnum) {
            return;
        }
    }

    if (!PXTRACE_G(output_file)) {
        if (!pxtrace_open_output()) {
            pxtrace_disable();
            return;
        }
    }

    fprintf(PXTRACE_G(output_file),
        "%20s "
        "%s% 4d%s "
        "%*s"
        "%s%s:%-4d%s "
        "%s%.*s%s\n",
        " ",
        pxtrace_color(PXTRACE_COLOR_DEPTH), PXTRACE_G(depth), pxtrace_color(PXTRACE_COLOR_RESET),
        (PXTRACE_G(depth) < 0 ? 0 : PXTRACE_G(depth)) * 2, "",
        pxtrace_color(PXTRACE_COLOR_PATH), fpath, lineno, pxtrace_color(PXTRACE_COLOR_RESET),
        pxtrace_color(PXTRACE_COLOR_CODE), linelen > 0 ? linelen - 1 : 1, linelen > 0 ? linep : "-", pxtrace_color(PXTRACE_COLOR_RESET)
    );

}

static int pxtrace_read_line(char *fpath, int lineno, char *line, size_t nline) {
    int linelen;
    FILE *f = fopen(fpath, "r");
    if (!f) return -1;

    do {
        if ((linelen = getline(&line, &nline, f)) < 0) {
            fclose(f);
            return -1;
        }
    } while (--lineno);
    fclose(f);
    return linelen;
}

static int pxtrace_open_output(void) {
    // Note: Specifying `/dev/stderr` or `/dev/stdout` also works but it flushes
    //       differently from the already opened stdio FILE pointers, leading to
    //       disordered output if your scripts also write to stdout or stderr.
    //       That's the reason for supporting these special `@stderr` and
    //       `@stdout` paths.
    if (strcmp(PXTRACE_G(output_path), "@stderr") == 0) {
        PXTRACE_G(output_file) = stderr;
    } else if (strcmp(PXTRACE_G(output_path), "@stdout") == 0) {
        PXTRACE_G(output_file) = stdout;
    } else {
        PXTRACE_G(output_file) = fopen(PXTRACE_G(output_path), "w");
        if (!PXTRACE_G(output_file)) {
            php_error_docref(NULL, E_WARNING, "pxtrace: Failed to open path %s; disabling", PXTRACE_G(output_path));
            return 0;
        }
        PXTRACE_G(output_fopened) = 1;
    }
    return 1;
}

static int pxtrace_get_current_stack_depth(void) {
    int depth = 0;
    zend_execute_data *e = EG(current_execute_data);
    while (e && e->prev_execute_data) {
        depth++;
        e = e->prev_execute_data;
    }
    return depth;
}

static void pxtrace_enable(void) {
    if (zend_execute_ex != pxtrace_execute_ex) {
        PXTRACE_G(orig_execute_ex) = execute_ex;
        zend_execute_ex = pxtrace_execute_ex;
    }
    PXTRACE_G(enabled) = 1;
    PXTRACE_G(last_ns) = 0;
    PXTRACE_G(depth) = pxtrace_get_current_stack_depth();
}

static void pxtrace_disable(void) {
    if (zend_execute_ex == pxtrace_execute_ex) {
        zend_execute_ex = PXTRACE_G(orig_execute_ex);
    }
    pxtrace_close_file();
    PXTRACE_G(enabled) = 0;
}

static void pxtrace_close_file(void) {
    if (PXTRACE_G(output_file) && PXTRACE_G(output_fopened)) {
        fclose(PXTRACE_G(output_file));
    }
    PXTRACE_G(output_file) = NULL;
    PXTRACE_G(output_fopened) = 0;
}

PHP_FUNCTION(pxtrace_set_enabled) {
    bool on_off = 0;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_BOOL(on_off)
    ZEND_PARSE_PARAMETERS_END();

    if (on_off) {
        pxtrace_enable();
    } else {
        pxtrace_disable();
    }
}

static char *pxtrace_color(int c) {
    if (!PXTRACE_G(ansi_color)) {
        return "";
    }
    switch (c) {
        case PXTRACE_COLOR_RESET:  return "\x1b[0m";
        case PXTRACE_COLOR_TIME:   return "\x1b[36m";
        case PXTRACE_COLOR_DEPTH:  return "\x1b[34m";
        case PXTRACE_COLOR_PATH:   return "";
        case PXTRACE_COLOR_FUNC:   return "\x1b[1;35m";
        case PXTRACE_COLOR_CODE:   return "\x1b[2m";
    }
    return "";
}
