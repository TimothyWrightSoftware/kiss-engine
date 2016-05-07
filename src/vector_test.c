#include "minunit.h"
#include "math.h"
#define STB_DEFINE
#include "vec.h"
#define KISS_VEC_IMP
#include "kiss_vec.h"

#define EPSILON	0.000001f
#define EQ(a,b) (fabs((a) - (b)) < EPSILON)

char* test_vec_add() {

	vec v0 = vec3( 1.0f, 2.0f, 3.0f );
	vec v1 = vec3( 2.0f, 3.0f, 5.0f );
	vec_add( &v0, &v0, &v1 );

	mu_assert( EQ( v0.x, 3.0f ), "x not equal" );
	mu_assert( EQ( v0.y, 5.0f ), "y not equal" );
	mu_assert( EQ( v0.z, 8.0f ), "z not equal" );

	v0 = vec3( 1.0f, 2.0f, 3.0f );
	v1 = vec3( 2.0f, 3.0f, 5.0f );
	vec_addeq( &v0, &v1 );

	mu_assert( EQ( v0.x, 3.0f ), "x not equal" );
	mu_assert( EQ( v0.y, 5.0f ), "y not equal" );
	mu_assert( EQ( v0.z, 8.0f ), "z not equal" );

	return NULL;
}

char* test_perspective_matrix() {

	float fovy = deg2rad( 45.0f );
	float aspect = 16.0f / 9.0f;
	float near = 0.1f;
	float far = 10.0f;
	float proj[16] ;
	kiss_perspective_f16( proj, fovy, aspect, near, far );

	float sine, cotangent, deltaZ;
	float radians = fovy / 2.0f;

	deltaZ = far - near;
	sine = sin(radians);
	cotangent = cos(radians) / sine;

	mu_assert( EQ( proj[0], cotangent / aspect ), "" );
	mu_assert( EQ( proj[5], cotangent ), "" );
	mu_assert( EQ( proj[10], -(far + near) / deltaZ), "" );
	mu_assert( EQ( proj[11], -1 ), "" );
	mu_assert( EQ( proj[14], -2 * near * far / deltaZ ), "" );
	mu_assert( EQ( proj[15], 0 ), "" );

	// test results here
	return NULL;
}
//TODO - more tests...
char* test_matrix_multiplication() {
	float a[] = { 1,  2,  3,  4,
		 5,  6,  7,  8,
		 9, 10, 11, 12,
		13, 14, 15, 16,
	};
	kiss_mat4 *A = (kiss_mat4*)a;
	float b[] = {
		17, 18, 19, 20,
		21, 22, 23, 24,
		25, 26, 27, 28,
		29, 30, 31, 32,
	};
	kiss_mat4 *B = (kiss_mat4*)b;
	kiss_mat4 R;
	kiss_mul_m4( &R, A, B );

	KISS_MAT4DBG( R, stdout );

	mu_assert( EQ( R.m[0], 250.0 ), "[0]" );
	mu_assert( EQ( R.m[1], 260.0 ), "[1]" );
	mu_assert( EQ( R.m[2], 270.0 ), "[2]" );
	mu_assert( EQ( R.m[3], 280.0 ), "[3]" );

	mu_assert( EQ( R.m[4], 618.0 ), "[4]" );
	mu_assert( EQ( R.m[5], 644.0 ), "[5]" );
	mu_assert( EQ( R.m[6], 670.0 ), "[6]" );
	mu_assert( EQ( R.m[7], 696.0 ), "[7]" );
	
	mu_assert( EQ( R.m[8], 986.0 ), "[8]" );
	mu_assert( EQ( R.m[9], 1028.0 ), "[9]" );
	mu_assert( EQ( R.m[10], 1070.0 ), "[10]" );
	mu_assert( EQ( R.m[11], 1112.0 ), "[11]" );
	
	mu_assert( EQ( R.m[12], 1354.0 ), "[12]" );
	mu_assert( EQ( R.m[13], 1412.0 ), "[13]" );
	mu_assert( EQ( R.m[14], 1470.0 ), "[14]" );
	mu_assert( EQ( R.m[15], 1528.0 ), "[15]" );
	
	return NULL;
}

char* test_mat4_identity() {
	kiss_mat4 mat;
	kiss_ident_m4( &mat );

	mu_assert( EQ( mat.m[0], 1.0 ), "0" );
	mu_assert( EQ( mat.m[1], 0.0 ), "1" );
	mu_assert( EQ( mat.m[2], 0.0 ), "2" );
	mu_assert( EQ( mat.m[3], 0.0 ), "3" );

	mu_assert( EQ( mat.m[4], 0.0 ), "4" );
	mu_assert( EQ( mat.m[5], 1.0 ), "5" );
	mu_assert( EQ( mat.m[6], 0.0 ), "6" );
	mu_assert( EQ( mat.m[7], 0.0 ), "7" );

	mu_assert( EQ( mat.m[8], 0.0 ), "8" );
	mu_assert( EQ( mat.m[9], 0.0 ), "9" );
	mu_assert( EQ( mat.m[10], 1.0 ), "10" );
	mu_assert( EQ( mat.m[11], 0.0 ), "11" );

	mu_assert( EQ( mat.m[12], 0.0 ), "12" );
	mu_assert( EQ( mat.m[13], 0.0 ), "13" );
	mu_assert( EQ( mat.m[14], 0.0 ), "14" );
	mu_assert( EQ( mat.m[15], 1.0 ), "15" );

	return NULL;
}

char *all_tests() {
	mu_suite_start();

	mu_run_test(test_vec_add);
	mu_run_test(test_perspective_matrix);
	mu_run_test( test_matrix_multiplication );
	mu_run_test( test_mat4_identity );

	return NULL;
}

RUN_TESTS(all_tests);
