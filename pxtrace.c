#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_pxtrace.h"
#include "pxtrace_arginfo.h"

ZEND_DECLARE_MODULE_GLOBALS(pxtrace)
static PHP_MINFO_FUNCTION(pxtrace);
static PHP_MINIT_FUNCTION(pxtrace);
static PHP_MSHUTDOWN_FUNCTION(pxtrace);
static PHP_INI_MH(pxtrace_on_update_output_path);
static void pxtrace_execute_ex(zend_execute_data *execute_data);
static int pxtrace_open_output(void);
static int pxtrace_get_current_stack_depth(void);
static void pxtrace_enable(void);
static void pxtrace_disable(void);
static void pxtrace_close_file(void);

zend_module_entry pxtrace_module_entry = {
    STANDARD_MODULE_HEADER,
    "pxtrace",
    ext_functions,
    PHP_MINIT(pxtrace),
    PHP_MSHUTDOWN(pxtrace),
    NULL,
    NULL,
    PHP_MINFO(pxtrace),
    PHP_PXTRACE_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PXTRACE
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(pxtrace)
#endif

PHP_INI_BEGIN()
        PHP_INI_ENTRY("pxtrace.output_path", "@stderr", PHP_INI_ALL, pxtrace_on_update_output_path)
    STD_PHP_INI_ENTRY("pxtrace.auto_enable", "0",       PHP_INI_ALL, OnUpdateLong, auto_enable, zend_pxtrace_globals, pxtrace_globals)
PHP_INI_END()

static PHP_MINFO_FUNCTION(pxtrace) {
    php_info_print_table_start();
    php_info_print_table_header(2, "pxtrace support", "enabled");
    php_info_print_table_header(2, "extension version", PHP_PXTRACE_VERSION);
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

static PHP_INI_MH(pxtrace_on_update_output_path) {
    pxtrace_close_file();
    PXTRACE_G(output_path) = ZSTR_VAL(new_value);
    return SUCCESS;
}

static void pxtrace_execute_ex(zend_execute_data *execute_data) {
    zend_function *zf = execute_data->func;

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
    zend_execute_data *prev_data = execute_data->prev_execute_data
        ? execute_data->prev_execute_data
        : NULL;
    if (prev_data) {
        zend_function *prev_zf = prev_data->func;
        if (prev_zf) {
            if (prev_zf->type == ZEND_USER_FUNCTION) {
                fpath = ZSTR_VAL(prev_zf->op_array.filename);
                lineno = prev_data->opline ? prev_data->opline->lineno : -1;
            }
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
        "%20.3f " "% 4d " "%*s" "%s:%d " "%s%s" "%s\n",
        (float)(PXTRACE_G(last_ns) == 0 ? 0 : (cur_ns - PXTRACE_G(last_ns))) / 1000.f,
        PXTRACE_G(depth),
        (PXTRACE_G(depth) < 0 ? 0 : PXTRACE_G(depth)) * 2, "",
        fpath, lineno,
        class_name, *class_name != '\0' ? "::" : "",
        fname
    );

    PXTRACE_G(last_ns) = cur_ns;

    PXTRACE_G(depth) += 1;
    (PXTRACE_G(orig_execute_ex))(execute_data);
    PXTRACE_G(depth) -= 1;
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
    PXTRACE_G(last_ns) = 0;
    PXTRACE_G(depth) = pxtrace_get_current_stack_depth();
}

static void pxtrace_disable(void) {
    if (zend_execute_ex == pxtrace_execute_ex) {
        zend_execute_ex = PXTRACE_G(orig_execute_ex);
    }
    pxtrace_close_file();
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
