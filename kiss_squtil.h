/*
 * squtil - version 0.1
 *
 * This is a utility file to help with squirrel script
 */

#ifndef SQUTIL_H
#define SQUTIL_H

#ifdef SQUTIL_STATIC
#define SQUTIL_DEF static
#else
#define SQUTIL_DEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <squirrel.h> 

// definitions go here...
SQUTIL_DEF SQRESULT get_string( HSQUIRRELVM v, const char* name, const SQChar** string );
SQUTIL_DEF SQRESULT get_int( HSQUIRRELVM v, const char* name, SQInteger* integer );
SQUTIL_DEF SQRESULT get_var( HSQUIRRELVM v, const char* name );
SQUTIL_DEF void printfunc( HSQUIRRELVM v, const SQChar *s, ... );
SQUTIL_DEF void errorfunc( HSQUIRRELVM v, const SQChar *s, ... );
SQUTIL_DEF void print_stack( HSQUIRRELVM v );
SQUTIL_DEF void register_global_func( HSQUIRRELVM v, SQFUNCTION f, const char *fname );

#ifdef __cplusplus
}
#endif

#endif // end SQUTIL_H

#ifdef SQUTIL_IMPLEMENTATION

#include <sqstdio.h> 
#include <sqstdaux.h> 
#include <iostream>
#include <cstdarg>

SQUTIL_DEF SQRESULT get_string( HSQUIRRELVM v, const char* name, const SQChar** string ) {
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushstring( v, _SC( name ), -1 );
	SQRESULT result = sq_get( v, -2 );
	if( SQ_SUCCEEDED( result ) ) {
		sq_getstring( v, -1, string );
	}
	sq_settop( v, top );
	return result;
}

SQUTIL_DEF SQRESULT get_int( HSQUIRRELVM v, const char* name, SQInteger* integer ) {
	SQInteger top = sq_gettop(v); //saves the stack size before the call
	sq_pushstring( v, _SC( name ), -1 );
	SQRESULT result = sq_get( v, -2 );
	if( SQ_SUCCEEDED( result ) ) {
		sq_getinteger( v, -1, integer );
	}
	sq_settop( v, top );
	return result;
}

SQUTIL_DEF void register_global_func( HSQUIRRELVM v, SQFUNCTION f, const char *fname ) {
	sq_pushstring(v,fname,-1);
	sq_newclosure(v,f,0); //create a new function
	sq_newslot(v,-3,SQFalse);
}

SQUTIL_DEF SQRESULT get_var( HSQUIRRELVM v, const char* name ) {
	sq_pushstring( v, _SC(name), -1 );
	return sq_get( v, -2 );
}

SQUTIL_DEF void printfunc( HSQUIRRELVM v, const SQChar *s, ... ) {
	va_list vl;
	va_start(vl, s);
	vfprintf(stdout, s, vl);
	va_end(vl);
}

SQUTIL_DEF void errorfunc( HSQUIRRELVM v, const SQChar *s, ... ) {
	va_list vl;
	va_start(vl, s);
	vfprintf(stderr, s, vl);
	va_end(vl);
}

static void print_integer( HSQUIRRELVM v, SQInteger index ) {
	SQInteger value;
	sq_getinteger( v, index, &value );
	std::cout <<value;
}

static void print_float( HSQUIRRELVM v, SQInteger index ) {
	SQFloat value;
	sq_getfloat( v, index, &value );
	std::cout << value;
}

static void print_string( HSQUIRRELVM v, SQInteger index ) {
	const SQChar* value;
	sq_getstring( v, index, &value );
	std::cout << value;
}

static void print_bool( HSQUIRRELVM v, SQInteger index ) {
	SQBool value;
	sq_getbool( v, index, &value );
	std::cout << value;
}

static void print_stack_value( HSQUIRRELVM v, SQObjectType type, SQInteger index ) {
	switch( type ) {
		case OT_NULL: std::cout << "NULL"; break;
		case OT_INTEGER: print_integer( v, index ); break;
		case OT_FLOAT: print_float( v, index ); break;
		case OT_STRING: print_string( v, index ); break;
		case OT_TABLE: std::cout << "TABLE"; break;
		case OT_ARRAY: std::cout << "ARRAY"; break;
		case OT_USERDATA: std::cout << "USERDATA"; break;
		case OT_CLOSURE: std::cout << "CLOSURE"; break;
		case OT_NATIVECLOSURE: std::cout << "NATIVECLOSURE"; break;
		case OT_GENERATOR: std::cout << "GENERATOR"; break;
		case OT_USERPOINTER: std::cout << "USERPOINTER"; break;
		case OT_BOOL: print_bool( v, index ); break;
		case OT_INSTANCE: std::cout << "INSTANCE"; break;
		case OT_CLASS: std::cout << "CLASS"; break;
		case OT_WEAKREF: std::cout << "WEAKREF"; break;
		default: std::cout << "UNKNOWN"; break;
	}

}

SQUTIL_DEF void print_stack(HSQUIRRELVM v) {

	SQInteger top = sq_gettop( v );
	std::cout << "Top: " << top << std::endl;
	for( int i = top; i > 0; --i ) {
		SQObjectType type = sq_gettype( v, i );
		print_stack_value( v, type, i );
		std::cout << std::endl;
	}
}

#endif // end SQUTIL_IMPLEMENTATION

