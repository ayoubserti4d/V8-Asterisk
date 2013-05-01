/*
* Asterisk -- A telephony toolkit for Linux.
*
* Echo application -- play back what you hear to evaluate latency
* 
* Copyright (C) 1999, Mark Spencer
*
* Mark Spencer <markster@linux-support.net>
*
* This program is free software, distributed under the terms of
* the GNU General Public License
*/
extern "C"
{
#include <asterisk.h>
  
#include <asterisk/lock.h>
#include <asterisk/file.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>

#include <asterisk/pbx.h>
#include <asterisk/module.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

}
#include <v8.h>
//ASTERISK_FILE_VERSION(__FILE__, "$Revision: 360033 $")

struct ast_app {
	char name[AST_MAX_APP];			
	int (*execute)(struct ast_channel *chan, void *data);
	char *synopsis;				
	char *description;			
	struct ast_app *next;			
};

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;
static char *tdesc = "Execute Javascript";

static char *app = "ExecJS";

static char *synopsis = "Execute Javascript Code";

static char *descrip = "ExecJS(): disallocate Extensions configuration from extensions.conf file.";


v8::Handle<v8::Value> Echo(const v8::Arguments& args);
v8::Handle<v8::Value> PlayBack(const v8::Arguments& args);
v8::Handle<v8::Value> Dial(const v8::Arguments& args);

//internal function
v8::Handle<v8::String> ReadFile(const char* name);
bool ExecuteString(v8::Handle<v8::String> source, v8::Handle<v8::Value> name, bool print_result, bool report_exceptions);

//global variable
struct ast_channel *Globalchan=NULL;
void *Globaldata = NULL;

static int execjs_exec(struct ast_channel *chan, void *data)
{

	//ast_log (LOG_NOTICE, "Hello world");
	Globalchan = chan;
	Globaldata = data;
	char *str = (char*)data;

	v8::HandleScope handle_scope;
	// Create a template for the global object.
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	global->Set(v8::String::New("echo"), v8::FunctionTemplate::New(Echo));
	global->Set(v8::String::New("playback"), v8::FunctionTemplate::New(PlayBack));
	global->Set(v8::String::New("dial"), v8::FunctionTemplate::New(dial));
	v8::Handle<v8::Context> context = v8::Context::New(NULL, global);
	v8::Context::Scope context_scope(context);


	v8::Handle<v8::String> file_name = v8::String::New(str);
	v8::Handle<v8::String> source = ReadFile(str);
	if (source.IsEmpty()) {
		ast_verbose("Error reading '%s'\n", str);

	}
	else
	{
		if (!ExecuteString(source, file_name, false, true))
		{
			ast_verbose("Error Executing file %s" , str);
		}
	}

	//ast_verbose ("\n %d \n" , result);	


	return 1;
}

const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

v8::Handle<v8::Value> Echo(const v8::Arguments& args) {

	ast_app* mAstApp = pbx_findapp("echo");
	if (mAstApp == NULL) 
	{
		ast_verbose("No Echo application");
		ast_log (LOG_NOTICE, "Faild");

	}
	else
	{
		
		int what = pbx_exec(Globalchan,mAstApp,NULL,10);

		ast_verbose ("%d\n %s \n" , what,"OK");
	}

	return v8::Undefined();
}

v8::Handle<v8::Value> PlayBack(const v8::Arguments& args) {

	ast_app* mAstApp = pbx_findapp("playback");
	if (mAstApp == NULL) 
	{
		ast_verbose("No playback application");
		ast_log (LOG_NOTICE, "Faild");

	}
	else
	{
        v8::String::Utf8Value value(args[0]->ToString());
		const char* fileArg = ToCString(value);
		ast_verbose ("%s\n", fileArg);
		int what = pbx_exec(Globalchan,mAstApp,(void*)fileArg,10);

		ast_verbose ("%d\n %s \n" , what,"OK");
	}

	return v8::Undefined();
}
v8::Handle<v8::Value> Dial(const v8::Arguments& args) {

	ast_app* mAstApp = pbx_findapp("dial");
	if (mAstApp == NULL) 
	{
		ast_verbose("No dial application");
		ast_log (LOG_NOTICE, "Faild");

	}
	else
	{
        v8::String::Utf8Value value(args[0]->ToString());
		const char* fileArg = ToCString(value);
		ast_verbose ("%s\n", fileArg);
		int what = pbx_exec(Globalchan,mAstApp,(void*)fileArg,10);

		ast_verbose ("%d\n %s \n" , what,"OK");
	}

	return v8::Undefined();
}



// Reads a file into a v8 string.
v8::Handle<v8::String> ReadFile(const char* name) {
	FILE* file = fopen(name, "rb");
	if (file == NULL) return v8::Handle<v8::String>();

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i = 0; i < size;) {
		int read = fread(&chars[i], 1, size - i, file);
		i += read;
	}
	fclose(file);
	v8::Handle<v8::String> result = v8::String::New(chars, size);
	delete[] chars;
	return result;
}


// Executes a string within the current v8 context.
bool ExecuteString(v8::Handle<v8::String> source,  v8::Handle<v8::Value> name, bool print_result, bool report_exceptions) {
	v8::HandleScope handle_scope;
	v8::TryCatch try_catch;
	v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
	if (script.IsEmpty()) {
		// Print errors that happened during compilation.
		if (report_exceptions)
			//ReportException(&try_catch);
			return false;
	} else {
		v8::Handle<v8::Value> result = script->Run();
		if (result.IsEmpty()) {
			// Print errors that happened during execution.
			if (report_exceptions)
				// ReportException(&try_catch);
				return false;
		} else {
			if (print_result && !result->IsUndefined()) {
				// If all went well and the result wasn't undefined then print
				// the returned value.
				v8::String::Utf8Value str(result);
				const char* cstr = ToCString(str);
				printf("%s\n", cstr);
			}
			return true;
		}
	}
}

int unload_module(void)
{

	return ast_unregister_application(app);
}

int load_module(void)
{
	return ast_register_application(app, execjs_exec,synopsis ,descrip);
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	int res;
	STANDARD_USECOUNT(res);
	return res;
}

char *key()
{
	return ASTERISK_GPL_KEY;
}



//AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY,"ExecJS");
