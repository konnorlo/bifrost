if(NOT DEFINED VST3_BUNDLE)
    message(FATAL_ERROR "VST3_BUNDLE is required")
endif()

set(moduleinfo "${VST3_BUNDLE}/Contents/Resources/moduleinfo.json")

if(EXISTS "${moduleinfo}")
    execute_process(
        COMMAND /usr/bin/perl -0pi -e "s/,([\\s\\r\\n]*[}\\]])/$1/g" "${moduleinfo}"
        RESULT_VARIABLE sanitize_result
    )

    if(NOT sanitize_result EQUAL 0)
        message(FATAL_ERROR "Failed to sanitize ${moduleinfo}")
    endif()
endif()
