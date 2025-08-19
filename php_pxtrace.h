#ifndef PHP_PXTRACE_H
#define PHP_PXTRACE_H

extern zend_module_entry pxtrace_module_entry;
#define phpext_pxtrace_ptr &pxtrace_module_entry

#define PHP_PXTRACE_VERSION "0.1.0"

ZEND_BEGIN_MODULE_GLOBALS(pxtrace)
    char *output_path;
    FILE *output_file;
    int output_fopened;
    int auto_enable;
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
