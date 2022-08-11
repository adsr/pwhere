#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_pwhere.h"

ZEND_DLEXPORT void pwhere_stmt_handler(zend_execute_data *frame) {
}

ZEND_EXT_API zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    (char*)ZEND_EXTENSION_BUILD_ID
};

ZEND_DLEXPORT zend_extension zend_extension_entry = {
    (char*)"pwhere",
    (char*)"0.1.0",
    (char*)"Adam Saponara",
    (char*)"https://github.com/adsr/pwhere",
    (char*)"MIT",
    NULL,           /* startup */
    NULL,           /* shutdown */
    NULL,           /* activate_func_t */
    NULL,           /* deactivate_func_t */
    NULL,           /* message_handler_func_t */
    NULL,           /* op_array_handler_func_t */
    pwhere_stmt_handler, /* statement_handler_func_t */
    NULL,           /* fcall_begin_handler_func_t */
    NULL,           /* fcall_end_handler_func_t */
    NULL,           /* op_array_ctor_func_t */
    NULL,           /* op_array_dtor_func_t */
    STANDARD_ZEND_EXTENSION_PROPERTIES
};
