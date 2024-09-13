#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_pwhere.h"

static void (*orig_execute_ex)(zend_execute_data *execute_data) = NULL;

static int pwhere_depth = 0;
static uint64_t pwhere_last_ns = 0;

static void pwhere_execute_ex(zend_execute_data *execute_data) {
    zend_function *zf = execute_data->func;

    char *fname = pwhere_depth == 0 ? "<main>" : "<require>";
    if (zf->common.function_name) {
        fname = ZSTR_VAL(zf->common.function_name);
    }

    char *clazz = "";
    if (zf->common.scope) {
        clazz = ZSTR_VAL(zf->common.scope->name);
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

    fprintf(stderr,
        "%20.3f " "%04x " "%*s" "%s:%d " "%s%s" "%s\n",
        (float)(pwhere_last_ns == 0 ? 0 : (cur_ns - pwhere_last_ns)) / 1000.f,
        pwhere_depth,
        pwhere_depth * 2, "",
        fpath, lineno,
        clazz, *clazz != '\0' ? "::" : "",
        fname
    );

    pwhere_last_ns = cur_ns;

    ++pwhere_depth;
    orig_execute_ex(execute_data);
    --pwhere_depth;
}

PHP_MINIT_FUNCTION(pwhere) {
    orig_execute_ex = execute_ex;
    zend_execute_ex = pwhere_execute_ex;
}

PHP_MSHUTDOWN_FUNCTION(pwhere) {
    zend_execute_ex = orig_execute_ex;
}

PHP_MINFO_FUNCTION(pwhere) {
    php_info_print_table_start();
    php_info_print_table_header(2, "pwhere support", "enabled");
    php_info_print_table_end();
}

zend_module_entry pwhere_module_entry = {
    STANDARD_MODULE_HEADER,
    "pwhere",
    NULL,
    PHP_MINIT(pwhere),
    PHP_MSHUTDOWN(pwhere),
    NULL,
    NULL,
    PHP_MINFO(pwhere),
    PHP_PWHERE_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PWHERE
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(pwhere)
#endif
