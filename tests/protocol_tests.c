#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "../src/packet/packet.h"

int init_suite(void){
	return 0;
}

int clean_suite(void){
	return 0;
}

void test_send_receive(){
    CU_ASSERT_EQUAL(1,1);
}

int main(){
	printf("-----------------------------------------------------------\n");
	if (CUE_SUCCESS != CU_initialize_registry()){
        return CU_get_error();
    }

    CU_pSuite pSuite = NULL;
	pSuite = CU_add_suite("TESTS : protocol_tests.c", init_suite, clean_suite);
	if (pSuite == NULL) {
	    CU_cleanup_registry();
        return CU_get_error();
	}
	
	if (CU_add_test(pSuite, "Tests d'envoi/reception", test_send_receive) == NULL)
	{
        CU_cleanup_registry();
        return CU_get_error();
	}
	
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_basic_show_failures(CU_get_failure_list());

	CU_cleanup_registry();
	printf("-----------------------------------------------------------\n");
	return CU_get_error();
}