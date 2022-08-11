#ifndef PHP_PWHERE_H
#define PHP_PWHERE_H

extern zend_module_entry pwhere_module_entry;
#define phpext_pwhere_ptr &pwhere_module_entry

#define PHP_PWHERE_VERSION "0.1.0"

#if defined(ZTS) && defined(COMPILE_DL_PWHERE)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif /* PHP_PWHERE_H */
