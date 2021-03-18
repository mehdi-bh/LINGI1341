#include <stdlib.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>
#include <CUnit/Automated.h>
#include <CUnit/CUnit.h>

#include "../src/buffer/buffer.h"
#include "../src/packet/packet.h"

int init_suite(void){
    return 0;
}

int clean_suite(void){
    return 0;
}

void test_buffer_enqueue(void){
    /*** Init ***/
    buffer_t* buffer = buffer_init();

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, 11111);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, 22222);

    /*** Packet 3 ***/
    pkt_t* packet3 = pkt_new();
    pkt_set_seqnum(packet3, 3);
    pkt_set_timestamp(packet3, 33333);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);
    buffer_enqueue(buffer, packet3);

    /*** Check if packets are added ***/
    CU_ASSERT_EQUAL(pkt_get_seqnum(buffer->first->pkt), 1);
    CU_ASSERT_EQUAL(pkt_get_seqnum(buffer->first->next->pkt), 2);
    CU_ASSERT_EQUAL(pkt_get_seqnum(buffer->first->next->next->pkt), 3);

    /*** Free buffer ***/
    buffer_free(buffer);
}

void test_buffer_size(void){
    /*** Init ***/
    buffer_t* buffer = buffer_init();

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, 11111);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, 22222);

    /*** Packet 3 ***/
    pkt_t* packet3 = pkt_new();
    pkt_set_seqnum(packet3, 3);
    pkt_set_timestamp(packet3, 33333);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);
    buffer_enqueue(buffer, packet3);

    /*** Check buffer size ***/
    CU_ASSERT_EQUAL(buffer_size(buffer), 3);
    CU_ASSERT_NOT_EQUAL(buffer_size(buffer), 5);

    /*** Free buffer ***/
    buffer_free(buffer);
}

void test_buffer_remove_first(void){
    /*** Init ***/
    buffer_t* buffer = buffer_init();

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, 11111);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, 22222);

    /*** Packet 3 ***/
    pkt_t* packet3 = pkt_new();
    pkt_set_seqnum(packet3, 3);
    pkt_set_timestamp(packet3, 33333);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);
    buffer_enqueue(buffer, packet3);

    /*** Remove packet 1 and test ***/
    pkt_t* pkt = buffer_remove_first(buffer);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 1);
    CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 11111);
    CU_ASSERT_EQUAL(buffer_size(buffer), 2);

    /*** Remove packet 2 and test ***/
    pkt = buffer_remove_first(buffer);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 2);
    CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 22222);
    CU_ASSERT_EQUAL(buffer_size(buffer), 1);

    /*** Remove packet 3 and test ***/
    pkt = buffer_remove_first(buffer);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 3);
    CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 33333);
    CU_ASSERT_EQUAL(buffer_size(buffer), 0);

    /*** NULL PACKET ***/
    pkt = buffer_remove_first(buffer);
    CU_ASSERT_EQUAL(pkt, NULL);

    /*** Free buffer ***/
    pkt_del(pkt);
    buffer_free(buffer);
}

void test_buffer_remove(void){
    /*** Init ***/
    buffer_t* buffer = buffer_init();

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, 11111);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, 22222);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);

    /*** Remove packet 2 and test ***/
    pkt_t* pkt = buffer_remove(buffer, 2);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 2);
    CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 22222);
    CU_ASSERT_EQUAL(buffer_size(buffer), 1);

    /*** Remove packet 1 and test ***/
    pkt = buffer_remove(buffer, 1);
    CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), 1);
    CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 11111);
    CU_ASSERT_EQUAL(buffer_size(buffer), 0);

    /*** Free buffer ***/
    pkt_del(pkt);
    buffer_free(buffer);
}

void test_buffer_remove_acked(void){
    /*** Init ***/
    buffer_t *buffer = buffer_init();

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, 11111);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, 22222);

    /*** Packet 3 ***/
    pkt_t* packet3 = pkt_new();
    pkt_set_seqnum(packet3, 3);
    pkt_set_timestamp(packet3, 33333);

    /*** Packet 4 ***/
    pkt_t* packet4 = pkt_new();
    pkt_set_seqnum(packet4, 4);
    pkt_set_timestamp(packet4, 44444);

    /*** Packet 5 ***/
    pkt_t* packet5 = pkt_new();
    pkt_set_seqnum(packet5, 5);
    pkt_set_timestamp(packet5, 55555);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);
    buffer_enqueue(buffer, packet3);
    buffer_enqueue(buffer, packet4);
    buffer_enqueue(buffer, packet5);

    /*** Remove packets until n°3 ***/
    int count = buffer_remove_acked(buffer, 3);

    /*** Check count, buffer size and that the good packets have been removed ***/
    CU_ASSERT_EQUAL(count, 2);
    CU_ASSERT_EQUAL(buffer_size(buffer), 3);
    CU_ASSERT_EQUAL(pkt_get_timestamp(buffer->first->pkt), 33333);

    /*** Free buffer ***/
    buffer_free(buffer);
}

void test_buffer_get_pkt(void){
    /*** Init ***/
    buffer_t *buffer = buffer_init();

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, 11111);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, 22222);

    /*** Packet 3 ***/
    pkt_t* packet3 = pkt_new();
    pkt_set_seqnum(packet3, 3);
    pkt_set_timestamp(packet3, 33333);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);
    buffer_enqueue(buffer, packet3);

    /*** Test if good packet returned ***/
    CU_ASSERT_EQUAL(pkt_get_timestamp(buffer_get_pkt(buffer,1)), 11111);
    CU_ASSERT_EQUAL(pkt_get_timestamp(buffer_get_pkt(buffer,2)), 22222);
    CU_ASSERT_EQUAL(pkt_get_timestamp(buffer_get_pkt(buffer,3)), 33333);
    CU_ASSERT_EQUAL(buffer_get_pkt(buffer,4), NULL);
    CU_ASSERT_EQUAL(buffer_get_pkt(buffer,0), NULL);

    /*** Free buffer ***/
    buffer_free(buffer);
}

void test_buffer_timedout_packet(void){
    /*** Init ***/
    buffer_t *buffer = buffer_init();
    uint32_t now_s = (uint32_t) time(NULL);

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, now_s - 10);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, now_s - 10);

    /*** Packet 3 ***/
    pkt_t* packet3 = pkt_new();
    pkt_set_seqnum(packet3, 3);
    pkt_set_timestamp(packet3, now_s - 10);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);
    buffer_enqueue(buffer, packet3);

    /*** Test if good packet returned ***/
    CU_ASSERT_EQUAL(pkt_get_seqnum(look_for_timedout_packet(buffer)), 1);
    CU_ASSERT_EQUAL(pkt_get_seqnum(look_for_timedout_packet(buffer)), 2);
    CU_ASSERT_EQUAL(pkt_get_seqnum(look_for_timedout_packet(buffer)), 3);

    CU_ASSERT_EQUAL(look_for_timedout_packet(buffer), NULL);

    /*** Free buffer ***/
    buffer_free(buffer);
}

void test_buffer_contains(void){
    /*** Init ***/
    buffer_t *buffer = buffer_init();

    /*** Packet 1 ***/
    pkt_t* packet1 = pkt_new();
    pkt_set_seqnum(packet1, 1);
    pkt_set_timestamp(packet1, 11111);

    /*** Packet 2 ***/
    pkt_t* packet2 = pkt_new();
    pkt_set_seqnum(packet2, 2);
    pkt_set_timestamp(packet2, 22222);

    /*** Add packets ***/
    buffer_enqueue(buffer, packet1);
    buffer_enqueue(buffer, packet2);

    /*** Check if contains ***/
    CU_ASSERT_TRUE(is_in_buffer(buffer, 1));
    CU_ASSERT_TRUE(is_in_buffer(buffer, 2));
    CU_ASSERT_FALSE(is_in_buffer(buffer, 99));
    CU_ASSERT_FALSE(is_in_buffer(buffer, 100));

    /*** Free buffer ***/
    buffer_free(buffer);
}

int main(){
    printf("-----------------------------------------------------------\n");
    if (CUE_SUCCESS != CU_initialize_registry()){
        return CU_get_error();
    }

    CU_pSuite pSuite = NULL;
        
    pSuite = CU_add_suite("TESTS : buffer.c", init_suite, clean_suite);
    if (pSuite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if  (
            CU_add_test(pSuite, "Tests de enqueue \n", test_buffer_enqueue) == NULL
            ||
            CU_add_test(pSuite, "Tests de size \n", test_buffer_size) == NULL
            ||
            CU_add_test(pSuite, "Tests de remove \n", test_buffer_remove) == NULL
            ||
            CU_add_test(pSuite, "Tests de remove_first \n", test_buffer_remove_first) == NULL
            ||
            CU_add_test(pSuite, "Tests de remove acked\n", test_buffer_remove_acked) == NULL
            ||
            CU_add_test(pSuite, "Tests de get pkt\n", test_buffer_get_pkt) == NULL
            ||
            CU_add_test(pSuite, "Tests de contains\n", test_buffer_contains) == NULL
            ||
            CU_add_test(pSuite, "Tests de look for timed out packet\n", test_buffer_timedout_packet) == NULL
        )
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