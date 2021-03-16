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

void test_pkt_getters_setters(){
    /*** New ***/
	CU_ASSERT_PTR_NOT_EQUAL(pkt_new(), NULL);
    pkt_t* pkt = pkt_new();

	/*** Type ***/
	CU_ASSERT_EQUAL(pkt_set_type(pkt, 4), E_TYPE);
    CU_ASSERT_EQUAL(pkt_set_type(pkt, 0), E_TYPE);
    CU_ASSERT_EQUAL(pkt_set_type(pkt, -500), E_TYPE);
    CU_ASSERT_EQUAL(pkt_set_type(pkt, 500), E_TYPE);
    
    // OK ACK
    CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_ACK), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_ACK);

    // OK NACK
    CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_NACK), PKT_OK);	
    CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_NACK);

    // OK DATA
    CU_ASSERT_EQUAL(pkt_set_type(pkt, PTYPE_DATA), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);

    /*** TR ***/
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 2), E_TR);
    CU_ASSERT_EQUAL(pkt_set_tr(pkt, 240), E_TR);

    // OK -> 0
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, 0), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_tr(pkt), 0);

    // OK -> 1
    CU_ASSERT_EQUAL(pkt_set_tr(pkt, 1), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), 1);

	/*** Window ***/
	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE + 1), E_WINDOW);

    // OK -> 0
    CU_ASSERT_EQUAL(pkt_set_window(pkt, 0), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_window(pkt), 0);

    // OK -> MAX_WINDOW_SIZE
	CU_ASSERT_EQUAL(pkt_set_window(pkt, MAX_WINDOW_SIZE), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), MAX_WINDOW_SIZE);

    /*** Seqnum ***/
	// OK -> 0
    CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 0), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 0);

    // OK -> 200
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, 200), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 200);
	
    /*** Length ***/
	CU_ASSERT_EQUAL(pkt_set_length(pkt, MAX_PAYLOAD_SIZE + 1), E_LENGTH);
    CU_ASSERT_EQUAL(pkt_set_length(pkt, 1500), E_LENGTH);

    // OK -> 100
	CU_ASSERT_EQUAL(pkt_set_length(pkt, 100), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_length(pkt), 100);

    // OK -> MAX_PAYLOAD_SIZE
	CU_ASSERT_EQUAL(pkt_set_length(pkt, MAX_PAYLOAD_SIZE), PKT_OK);
    CU_ASSERT_EQUAL(pkt_get_length(pkt), MAX_PAYLOAD_SIZE);

	/*** Timestamp ***/
    // OK -> 11111
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 11111), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 11111);

    // OK -> 55555
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, 55555), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 55555);

    /*** CRC1 ***/
    // OK -> 11111
	CU_ASSERT_EQUAL(pkt_set_crc1(pkt, 11111), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc1(pkt), 11111);

    // OK -> 55555
	CU_ASSERT_EQUAL(pkt_set_crc1(pkt, 55555), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc1(pkt), 55555);

    /*** CRC2 ***/
    // OK -> 11111
	CU_ASSERT_EQUAL(pkt_set_crc2(pkt, 11111), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc2(pkt), 11111);

    // OK -> 55555
	CU_ASSERT_EQUAL(pkt_set_crc2(pkt, 55555), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_crc2(pkt), 55555);

    /*** Payload ***/
	// Test 1
	char* payload = "SalutSalutArbitre";
	int length = strlen(payload);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, payload, length), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), length);
	CU_ASSERT_EQUAL(memcmp(pkt_get_payload(pkt), payload, length), 0);

	char* payload2 = "SalutSalutArbitre";
	payload = "zzzzzzzzzzzzzzzzzz";
	CU_ASSERT_NOT_EQUAL(memcmp(pkt_get_payload(pkt), payload, length), 0);
	CU_ASSERT_EQUAL(memcmp(pkt_get_payload(pkt), payload2, length), 0);
    
    // Test 2
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, NULL, 0), PKT_OK);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), 0);

    // Test 3
    CU_ASSERT_EQUAL(pkt_set_payload(pkt, NULL, MAX_PAYLOAD_SIZE + 1), E_LENGTH);
    
    /*** Delete ***/
	pkt_del(pkt);
}

void test_pkt_encode_decode(){
    /*** New ***/
	pkt_t* pkt = pkt_new();
	CU_ASSERT_PTR_NOT_EQUAL(pkt, NULL);

    /*** Datas to encode ***/
    uint8_t type = PTYPE_DATA;
    uint8_t tr = 0;
    uint8_t seqnum = 69;
    uint8_t window = 23;
    char* payload = "SalutSalutArbitre";
	int length = strlen(payload);
    uint32_t timestamp = 10012000;

    /*** Set packet ***/
	CU_ASSERT_EQUAL(pkt_set_type(pkt, type), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_tr(pkt, tr), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_seqnum(pkt, seqnum), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_window(pkt, window), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_payload(pkt, payload, length), PKT_OK);
	CU_ASSERT_EQUAL(pkt_set_timestamp(pkt, timestamp), PKT_OK);

    /*** Encode ***/
	size_t len = 50;
	size_t error = 10;
	char buffer[len];
    // Errors
    pkt_set_tr(pkt, 1);
    pkt_set_type(pkt, 2);
    CU_ASSERT_EQUAL(pkt_encode(pkt, buffer, &len), E_UNCONSISTENT);
    pkt_set_type(pkt, 3);
    CU_ASSERT_EQUAL(pkt_encode(pkt, buffer, &len), E_UNCONSISTENT);
    pkt_set_type(pkt, type);
	pkt_set_tr(pkt, tr);
    CU_ASSERT_EQUAL(pkt_encode(NULL, buffer, &len), E_UNCONSISTENT);
    CU_ASSERT_EQUAL(pkt_encode(pkt, NULL, &len), E_NOMEM);
    CU_ASSERT_EQUAL(pkt_encode(pkt, buffer, NULL), E_LENGTH);

    // OK
	CU_ASSERT_EQUAL(pkt_encode(pkt, buffer, &len), PKT_OK);

    /*** Decode ***/
	pkt_t* pkt_test = pkt_new();
	CU_ASSERT_EQUAL(pkt_decode(buffer, len, pkt_test), PKT_OK);

    /*** Compare packets ***/
	CU_ASSERT_EQUAL(pkt_get_type(pkt), type);
	CU_ASSERT_EQUAL(pkt_get_tr(pkt), tr);
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), seqnum);
	CU_ASSERT_EQUAL(pkt_get_window(pkt), window);
	CU_ASSERT_EQUAL(pkt_get_length(pkt), length);
	CU_ASSERT_EQUAL(memcmp(payload, pkt_get_payload(pkt), length), 0);
	CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), timestamp);

    /*** Del ***/
	pkt_del(pkt);
	pkt_del(pkt_test);
}

int main(){
	if (CUE_SUCCESS != CU_initialize_registry()){
        return CU_get_error();
    }

    CU_pSuite pSuite = NULL;
	pSuite = CU_add_suite("TESTS : packet.c", init_suite, clean_suite);
	if (pSuite == NULL) {
	    CU_cleanup_registry();
        return CU_get_error();
	}
	
	if (
		CU_add_test(pSuite, "Tests des get-set", test_pkt_getters_setters) == NULL 
		|| 
		CU_add_test(pSuite, "Tests des encode-decode", test_pkt_encode_decode) == NULL)
	{
        CU_cleanup_registry();
        return CU_get_error();
	}
	
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_basic_show_failures(CU_get_failure_list());

	CU_cleanup_registry();
	return CU_get_error();
}