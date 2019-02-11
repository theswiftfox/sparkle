#ifndef SPARKLE_CHAKRA_CORE_H
#define SPARKLE_CHAKRA_CORE_H

#include "Jsrt/ChakraCore.h"

#ifndef _WIN32
#include <cstring>
#include <memory>

#ifdef WITH_WSTRING
#include <string>
// TODO: move this stuff to some header eg. "ChakraCoreLinuxBridge.h"
JsErrorCode JsRunScript(const wchar_t * script, JsSourceContext sourceContext, const wchar_t *sourceUrl, JsValueRef * result) {
	JsValueRef scriptSource;
    JsCreateExternalArrayBuffer((void*)script, (unsigned int)wcslen(script), nullptr, nullptr, &scriptSource);
    // Run the script.
	JsValueRef placeholder;
	JsCreateString("placeholder", strlen("placeholder"), &placeholder);
	return JsRun(scriptSource, sourceContext++, placeholder, JsParseScriptAttributeNone, result);
}
#endif

JsErrorCode JsRunScript(const char* script, JsSourceContext sourceContext, const char* sourceUlr, JsValueRef* result) {
	JsValueRef scriptSource;
	JsCreateExternalArrayBuffer((void*)script, (unsigned int)strlen(script), nullptr, nullptr, &scriptSource);
	JsValueRef placeholder;
	JsCreateString("placeholder", strlen("placeholder"), &placeholder);
	return JsRun(scriptSource, sourceContext++, placeholder, JsParseScriptAttributeNone, result);
}

JsErrorCode JsStringToPointer(JsValueRef value, const char **stringValue, size_t *stringLength) {
	char *resultSTR = nullptr;
	auto err = JsErrorCode::JsNoError;
    err = JsCopyString(value, nullptr, 0, stringLength);
	if (err != JsNoError) {
		return err;
	}
    resultSTR = (char*) malloc(*stringLength + 1);
    err = JsCopyString(value, resultSTR, *stringLength + 1, nullptr);
	resultSTR[*stringLength] = 0;

	*stringValue = resultSTR;

	return err;
}

#endif

#endif