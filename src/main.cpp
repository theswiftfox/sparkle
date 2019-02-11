#include "Application.h"

#include "Jsrt/ChakraCore.h"
#include "Jsrt/ChakraCommonWindows.h"

#include <string>
#include <iostream>
#include <stdexcept>


#ifndef _WIN32
// TODO: move this stuff to some header eg. "ChakraCoreLinuxBridge.h"
JsErrorCode JsRunScript(const wchar_t * script, JsSourceContext sourceContext, const wchar_t *sourceUrl, JsValueRef * result) {
	JsValueRef scriptSource;
    JsCreateExternalArrayBuffer((void*)script, (unsigned int)wcslen(script), nullptr, nullptr, &scriptSource);
    // Run the script.
	JsValueRef placeholder;
	JsCreateString("placeholder", strlen("placeholder"), &placeholder);
	return JsRun(scriptSource, sourceContext++, placeholder, JsParseScriptAttributeNone, result);
}

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

int main(const int argc, char** argv) {
	JsRuntimeHandle runtime;
	JsContextRef context;
	JsValueRef result;
	unsigned currentSourceContext = 0;

	// Your script; try replace hello-world with something else
	std::string script = "(()=>{return \'Hello world!\';})()";

	// Create a runtime. 
	JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime);

	// Create an execution context. 
	JsCreateContext(runtime, &context);

	// Now set the current execution context.
	JsSetCurrentContext(context);

	// Run the script.
	auto runerr = JsRunScript(script.c_str(), currentSourceContext++, "", &result);

	// Convert your script result to String in JavaScript; redundant if your script returns a String
	JsValueRef resultJSString;
	JsConvertValueToString(result, &resultJSString);

	// Project script result back to C++.
	const char *resultChars;
	size_t stringLength;
	JsStringToPointer(resultJSString, &resultChars, &stringLength);

	std::string resultStr(resultChars);
	std::cout << resultStr << std::endl;

	// Dispose runtime
	JsSetCurrentContext(JS_INVALID_REFERENCE);
	JsDisposeRuntime(runtime);

	auto& app = Engine::App::getHandle();

	try {
		std::string config = "assets/settings.ini";
		if (argc > 1) {
			config = argv[1];
		}
		app.run(config);
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl << "Press any key to exit..." << std::endl;
		std::cin.ignore();
		return EXIT_FAILURE;
	}
	std::cin.ignore();
	return EXIT_SUCCESS;
}