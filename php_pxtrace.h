#ifndef PHP_PXTRACE_H
#define PHP_PXTRACE_H

extern zend_module_entry pxtrace_module_entry;
#define phpext_pxtrace_ptr &pxtrace_module_entry

#define PXTRACE_NAME      "pxtrace"
#define PXTRACE_VERSION   "0.1.0"
#define PXTRACE_AUTHOR    "Adam Saponara"
#define PXTRACE_URL       "https://github.com/adsr/pxtrace"
#define PXTRACE_COPYRIGHT "Copyright (c) 2025"

ZEND_BEGIN_MODULE_GLOBALS(pxtrace)
    int enabled;
    char *output_path;
    FILE *output_file;
    int output_fopened;
    int auto_enable;
    int trace_statements;
    int ansi_color;
    void (*orig_execute_ex)(zend_execute_data *execute_data);
    int depth;
    uint64_t last_ns;
ZEND_END_MODULE_GLOBALS(pxtrace)

ZEND_EXTERN_MODULE_GLOBALS(pxtrace)
#define PXTRACE_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(pxtrace, v)

#if defined(ZTS) && defined(COMPILE_DL_PXTRACE)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif /* PHP_PXTRACE_H */
