/*
 * kiss_lib - version 0.1
 *
 * This is a template for single header file libraries
 */

#ifndef KISS_VEC_H
#define KISS_VEC_H

#ifdef KISS_VEC_STATIC
#define KISS_VEC_DEF static
#else
#define KISS_VEC_DEF extern
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define KISS_DOT4( v1, v1_index, v1_offset, v2, v2_index, v2_offset ) \
	v1[ v1_index + v1_offset * 0 ] * v2[ v2_index + v2_offset * 0 ] + \
	v1[ v1_index + v1_offset * 1 ] * v2[ v2_index + v2_offset * 1 ] + \
	v1[ v1_index + v1_offset * 2 ] * v2[ v2_index + v2_offset * 2 ] + \
	v1[ v1_index + v1_offset * 3 ] * v2[ v2_index + v2_offset * 3 ]

typedef struct { float m[16]; } kiss_mat4;
typedef struct { float v[3]; } kiss_vec3;
typedef struct { float v[4]; } kiss_vec4;

#define KISS_VEC3(x, y, z) {(x), (y), (z)}
#define KISS_VEC3_ZERO() KISS_VEC3( 0.0f, 0.0f, 0.0f )
#define KISS_VEC3_DBG(v, out) \
	fprintf( out, "[%f, %f, %f]\n", v.v[ 0], v.v[ 1], v.v[ 2] ) 
#define KISS_VEC3X(V) (V)->v[0], (V)->v[1], (V)->v[2]

#define KISS_VEC4(x, y, z, w) {(x), (y), (z), (w)}
#define KISS_VEC4_ZERO() KISS_VEC4( 0.0f, 0.0f, 0.0f, 0.0f ) 
#define KISS_VEC4_DBG(v, out) \
	fprintf( out, "[%f, %f, %f, %f]\n", v.v[ 0], v.v[ 1], v.v[ 2], v.v[ 3] ) 
#define KISS_VEC4X(V) (V)->v[0], (V)->v[1], (V)->v[2], (V)->v[3]

#define KISS_MAT4() { 1.0f, 0.0f, 0.0f, 0.0f, \
					  0.0f, 1.0f, 0.0f, 0.0f, \
					  0.0f, 0.0f, 1.0f, 0.0f, \
				      0.0f, 0.0f, 0.0f, 1.0f }

//TODO - better name for this
#define KISS_MAT4DBG(mat, out) do {\
	fprintf( out, "[%f, %f, %f, %f]\n", mat.m[ 0], mat.m[ 1], mat.m[ 2], mat.m[ 3] ); \
	fprintf( out, "[%f, %f, %f, %f]\n", mat.m[ 4], mat.m[ 5], mat.m[ 6], mat.m[ 7] ); \
	fprintf( out, "[%f, %f, %f, %f]\n", mat.m[ 8], mat.m[ 9], mat.m[10], mat.m[11] ); \
	fprintf( out, "[%f, %f, %f, %f]\n", mat.m[12], mat.m[13], mat.m[14], mat.m[15] ); \
} while( 0 )

// fn declarations go here

KISS_VEC_DEF void kiss_ident_m4( kiss_mat4 *mat );
KISS_VEC_DEF void kiss_trans_m4v3( kiss_mat4 *out, kiss_vec3 *trans );
KISS_VEC_DEF void kiss_trans_m4f3( kiss_mat4 *out, float* v );
KISS_VEC_DEF void kiss_mul_m4( kiss_mat4 *out, kiss_mat4 *mat1, kiss_mat4 *mat2 );
KISS_VEC_DEF void kiss_perspective_m4( kiss_mat4 *out, float fovy_radians, float aspect, float n, float f );
KISS_VEC_DEF void kiss_rotx_m4(kiss_mat4 *out, float ang);
KISS_VEC_DEF void kiss_roty_m4(kiss_mat4 *out, float ang);
KISS_VEC_DEF void kiss_rotz_m4(kiss_mat4 *out, float ang);
KISS_VEC_DEF void kiss_rot_axis_m4(kiss_mat4 *out, int axis, float ang);
KISS_VEC_DEF void kiss_perspective_m4(kiss_mat4 *out, float fovy_radians, float aspect, float n, float f);
KISS_VEC_DEF void kiss_ortho_m4(kiss_mat4 *out, float left, float right, float bottom, float top, float nearVal, float farVal);

KISS_VEC_DEF void kiss_ident_f16( float *mat );
KISS_VEC_DEF void kiss_trans_f16v3( float *out, kiss_vec3 *trans );
KISS_VEC_DEF void kiss_trans_f16f3( float *out, float* v);
KISS_VEC_DEF void kiss_rotx_f16(float *mat, float ang);
KISS_VEC_DEF void kiss_roty_f16(float *mat, float ang);
KISS_VEC_DEF void kiss_rotz_f16(float *mat, float ang);
KISS_VEC_DEF void kiss_rot_axis_f16(float *mat, int axis, float ang);
KISS_VEC_DEF void kiss_mul_f16( float *out, float *mat1, float *mat2 );
KISS_VEC_DEF void kiss_perspective_f16(float *out, float fovy_radians, float aspect, float n, float f);
KISS_VEC_DEF void kiss_ortho_f16(float *out, float left, float right, float bottom, float top, float nearVal, float farVal);

#ifdef __cplusplus
}
#endif

#endif // end KISS_VEC_H

#ifdef KISS_VEC_IMP

KISS_VEC_DEF void kiss_ident_m4( kiss_mat4* mat ) {
	kiss_ident_f16( (float*)mat );
}

KISS_VEC_DEF void kiss_trans_m4v3( kiss_mat4 *out, kiss_vec3 *trans ) {
	kiss_trans_f16v3( (float*)out, trans );
}

KISS_VEC_DEF void kiss_trans_m4f3( kiss_mat4 *out, float* v ) {
	kiss_trans_f16f3( (float*)out, v );
}

KISS_VEC_DEF void kiss_rotx_m4(kiss_mat4 *out, float ang) {
	kiss_rotx_f16( (float*)out, ang );
}

KISS_VEC_DEF void kiss_roty_m4(kiss_mat4 *out, float ang) {
	kiss_roty_f16( (float*)out, ang );
}

KISS_VEC_DEF void kiss_rotz_m4(kiss_mat4 *out, float ang) {
	kiss_rotz_f16( (float*)out, ang );
}

KISS_VEC_DEF void kiss_rot_axis_m4(kiss_mat4 *out, int axis, float ang) {
	kiss_rot_axis_f16( (float*)out, axis, ang );
}

KISS_VEC_DEF void kiss_perspective_m4(kiss_mat4 *out, float fovy_radians, float aspect, float n, float f) {
	kiss_perspective_f16((float*)out, fovy_radians, aspect, n, f);
}

KISS_VEC_DEF void kiss_ortho_m4(kiss_mat4 *out, float left, float right, float bottom, float top, float nearVal, float farVal) {
	kiss_ortho_f16((float*)out, left, right, bottom, top, nearVal, farVal);
}

KISS_VEC_DEF void kiss_ident_f16( float* mat ) {
	mat[ 0] = 1.0f; mat[ 1] = 0.0f; mat[ 2] = 0.0f; mat[ 3] = 0.0f;
	mat[ 4] = 0.0f; mat[ 5] = 1.0f; mat[ 6] = 0.0f; mat[ 7] = 0.0f;
	mat[ 8] = 0.0f; mat[ 9] = 0.0f; mat[10] = 1.0f; mat[11] = 0.0f;
	mat[12] = 0.0f; mat[13] = 0.0f; mat[14] = 0.0f; mat[15] = 1.0f;
}

KISS_VEC_DEF void kiss_trans_f16v3( float *out, kiss_vec3 *trans ) {
	out[12] = trans->v[0];
	out[13] = trans->v[1];
	out[14] = trans->v[2];
}

KISS_VEC_DEF void kiss_trans_f16f3( float *out, float* v) {
	out[12] = v[0];
	out[13] = v[1];
	out[14] = v[2];
}

KISS_VEC_DEF void kiss_rotx_f16(float *mat, float ang) {
   float s = (float) sin(ang), c = (float) cos(ang);
   mat[ 0] =  1; mat[ 1] =  0; mat[ 2] =  0; mat[ 3] = 0;
   mat[ 4] =  0; mat[ 5] =  c; mat[ 6] =  s; mat[ 7] = 0;
   mat[ 8] =  0; mat[ 9] = -s; mat[10] =  c; mat[11] = 0;
   mat[12] =  0; mat[13] =  0; mat[14] =  0; mat[15] = 1;
}

KISS_VEC_DEF void kiss_roty_f16(float *mat, float ang) {
   float s = (float) sin(ang), c = (float) cos(ang);
   mat[ 0] =  c; mat[ 1] =  0; mat[ 2] = -s; mat[ 3] = 0;
   mat[ 4] =  0; mat[ 5] =  1; mat[ 6] =  0; mat[ 7] = 0;
   mat[ 8] = +s; mat[ 9] =  0; mat[10] =  c; mat[11] = 0;
   mat[12] =  0; mat[13] =  0; mat[14] =  0; mat[15] = 1;
}

KISS_VEC_DEF void kiss_rotz_f16(float *mat, float ang) {
   float s = (float) sin(ang), c = (float) cos(ang);
   mat[ 0] =  c; mat[ 1] =  s; mat[ 2] =  0; mat[ 3] = 0;
   mat[ 4] = -s; mat[ 5] =  c; mat[ 6] =  0; mat[ 7] = 0;
   mat[ 8] =  0; mat[ 9] =  0; mat[10] =  1; mat[11] = 0;
   mat[12] =  0; mat[13] =  0; mat[14] =  0; mat[15] = 1;
}

KISS_VEC_DEF void kiss_rot_axis_f16(float *mat, int axis, float ang) {
	//@TODO - errors???
   //assert(axis >= 0 && axis <= 2);
   switch (axis) {
      case 0: kiss_rotx_f16(mat, ang); break;
      case 1: kiss_roty_f16(mat, ang); break;
      case 2: kiss_rotz_f16(mat, ang); break;
      default: kiss_ident_f16(mat);
   }
}

KISS_VEC_DEF void kiss_mul_m4( kiss_mat4 *out, kiss_mat4 *mat1, kiss_mat4 *mat2 ) {
	kiss_mul_f16( (float*)out, (float*)mat1, (float*)mat2 );
}

KISS_VEC_DEF void kiss_mul_f16( float *out, float *mat1, float *mat2 ) {
	
   float temp1[16], temp2[16];
   if (mat1 == out) { memcpy(temp1, mat1, sizeof(temp1)); mat1 = temp1; }
   if (mat2 == out) { memcpy(temp2, mat2, sizeof(temp2)); mat2 = temp2; }

	out[ 0] = KISS_DOT4( mat1, 0, 1, mat2, 0, 4 );
	out[ 1] = KISS_DOT4( mat1, 0, 1, mat2, 1, 4 );
	out[ 2] = KISS_DOT4( mat1, 0, 1, mat2, 2, 4 );
	out[ 3] = KISS_DOT4( mat1, 0, 1, mat2, 3, 4 );

	out[ 4] = KISS_DOT4( mat1, 4, 1, mat2, 0, 4 );
	out[ 5] = KISS_DOT4( mat1, 4, 1, mat2, 1, 4 );
	out[ 6] = KISS_DOT4( mat1, 4, 1, mat2, 2, 4 );
	out[ 7] = KISS_DOT4( mat1, 4, 1, mat2, 3, 4 );

	out[ 8] = KISS_DOT4( mat1, 8, 1, mat2, 0, 4 );
	out[ 9] = KISS_DOT4( mat1, 8, 1, mat2, 1, 4 );
	out[10] = KISS_DOT4( mat1, 8, 1, mat2, 2, 4 );
	out[11] = KISS_DOT4( mat1, 8, 1, mat2, 3, 4 );

	out[12] = KISS_DOT4( mat1, 12, 1, mat2, 0, 4 );
	out[13] = KISS_DOT4( mat1, 12, 1, mat2, 1, 4 );
	out[14] = KISS_DOT4( mat1, 12, 1, mat2, 2, 4 );
	out[15] = KISS_DOT4( mat1, 12, 1, mat2, 3, 4 );
}

KISS_VEC_DEF void kiss_perspective_f16(float *out, float fovy_radians, float aspect, float n, float f ) {

	 float sine, cotangent, deltaZ;
	 float radians = fovy_radians / 2.0f;

	 deltaZ = f - n;
	 sine = sin(radians);

	 // doesn't this need some kind of epsilon thingy???
	 //@TODO - not sure about error stuff
	 //if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
	//return;
	 //}

	 cotangent = cos(radians) / sine;

	 kiss_ident_f16( out );
	 out[ 0] = cotangent / aspect;
	 out[ 5] = cotangent;
	 out[10] = -(f + n) / deltaZ;
	 out[11] = -1.0f;
	 out[14] = -2.0f * n * f / deltaZ;
	 out[15] = 0.0f;
	
}

KISS_VEC_DEF void kiss_ortho_f16(float *out, float left, float right, float bottom, float top, float nearVal, float farVal) {

	kiss_ident_f16(out);
	out[0] = 2.0f / (right - left);
	out[5] = 2.0f / (top - bottom);
	out[10] = -2.0f / (farVal - nearVal);
	out[12] = -((right + left) / (right - left));
	out[13] = -((top + bottom) / (top - bottom));
	out[14] = -((farVal + nearVal) / (farVal - nearVal));

}

#endif // end KISS_VEC_IMPLEMENTATION




