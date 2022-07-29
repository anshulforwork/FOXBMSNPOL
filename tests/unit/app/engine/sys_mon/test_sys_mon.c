/**
 *
 * @copyright &copy; 2010 - 2022, Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * We kindly request you to use one or more of the following phrases to refer to
 * foxBMS in your hardware, software, documentation or advertising materials:
 *
 * - &Prime;This product uses parts of foxBMS&reg;&Prime;
 * - &Prime;This product includes parts of foxBMS&reg;&Prime;
 * - &Prime;This product is derived from foxBMS&reg;&Prime;
 *
 */

/**
 * @file    test_sys_mon.c
 * @author  foxBMS Team
 * @date    2020-04-02 (date of creation)
 * @updated 2022-07-28 (date of last update)
 * @version v1.4.0
 * @ingroup UNIT_TEST_IMPLEMENTATION
 * @prefix  TEST
 *
 * @brief   Tests for the sys_mon module
 *
 */

/*========== Includes =======================================================*/
#include "unity.h"
#include "Mockdiag.h"
#include "Mockfram.h"
#include "Mockfram_cfg.h"
#include "Mockos.h"
#include "Mocksys_mon_cfg.h"

#include "fassert.h"
#include "sys_mon.h"
#include "test_assert_helper.h"

/*========== Definitions and Implementations for Unit Test ==================*/
#define DUMMY_TASK_ID_0  (0)
#define DUMMY_TASK_ID_1  (1)
#define DUMMY_TASK_ID_2  (2)
#define DUMMY_CYCLE_TIME (10)
#define DUMMY_MAX_JITTER (1)

void TEST_SYSM_DummyCallback_0(SYSM_TASK_ID_e taskId) {
    TEST_ASSERT_EQUAL(DUMMY_TASK_ID_0, taskId);
}

void TEST_SYSM_DummyCallback_1(SYSM_TASK_ID_e taskId) {
    TEST_ASSERT_EQUAL(DUMMY_TASK_ID_1, taskId);
}

void TEST_SYSM_DummyCallback_2(SYSM_TASK_ID_e taskId) {
    TEST_ASSERT_EQUAL(DUMMY_TASK_ID_2, taskId);
}

SYSM_MONITORING_CFG_s sysm_ch_cfg[3] = {
    {DUMMY_TASK_ID_0,
     SYSM_ENABLED,
     DUMMY_CYCLE_TIME,
     DUMMY_MAX_JITTER,
     SYSM_RECORDING_ENABLED,
     SYSM_HANDLING_SWITCH_OFF_CONTACTOR,
     TEST_SYSM_DummyCallback_0},
    {DUMMY_TASK_ID_1,
     SYSM_DISABLED,
     DUMMY_CYCLE_TIME,
     DUMMY_MAX_JITTER,
     SYSM_RECORDING_ENABLED,
     SYSM_HANDLING_SWITCH_OFF_CONTACTOR,
     TEST_SYSM_DummyCallback_1},
    {DUMMY_TASK_ID_2,
     SYSM_ENABLED,
     DUMMY_CYCLE_TIME,
     DUMMY_MAX_JITTER,
     SYSM_RECORDING_DISABLED,
     SYSM_HANDLING_SWITCH_OFF_CONTACTOR,
     TEST_SYSM_DummyCallback_2},
};

/** placeholder variable for the FRAM entry of sys mon */
FRAM_SYS_MON_RECORD_s fram_sys_mon_record = {0};

/*========== Setup and Teardown =============================================*/
void setUp(void) {
    SYSM_NOTIFICATION_s *notifications            = TEST_SYSM_GetNotifications();
    notifications[DUMMY_TASK_ID_0].timestampEnter = 0;
    notifications[DUMMY_TASK_ID_0].timestampExit  = 0;
    notifications[DUMMY_TASK_ID_0].duration       = 0;
    notifications[DUMMY_TASK_ID_1].timestampEnter = 0;
    notifications[DUMMY_TASK_ID_1].timestampExit  = 0;
    notifications[DUMMY_TASK_ID_1].duration       = 0;
    notifications[DUMMY_TASK_ID_2].timestampEnter = 0;
    notifications[DUMMY_TASK_ID_2].timestampExit  = 0;
    notifications[DUMMY_TASK_ID_2].duration       = 0;

    fram_sys_mon_record.anyTimingIssueOccurred              = false;
    fram_sys_mon_record.taskEngineViolatingDuration         = 0u;
    fram_sys_mon_record.taskEngineEnterTimestamp            = 0u;
    fram_sys_mon_record.task1msViolatingDuration            = 0u;
    fram_sys_mon_record.task1msEnterTimestamp               = 0u;
    fram_sys_mon_record.task10msViolatingDuration           = 0u;
    fram_sys_mon_record.task10msEnterTimestamp              = 0u;
    fram_sys_mon_record.task100msViolatingDuration          = 0u;
    fram_sys_mon_record.task100msEnterTimestamp             = 0u;
    fram_sys_mon_record.task100msAlgorithmViolatingDuration = 0u;
    fram_sys_mon_record.task100msAlgorithmEnterTimestamp    = 0u;
}

void tearDown(void) {
}

/*========== Test Cases =====================================================*/
void testSYSM_CheckNotificationsEarlyExitOnTimestampEquality(void) {
    /* Internal timestamp should be 0, expect an early return
       (if not returning early, test would fail since other functions
       are called; also make sure that the code path that would call the
       DIAG_Handler would be hit if not returning early) */
    SYSM_NOTIFICATION_s *notifications            = TEST_SYSM_GetNotifications();
    notifications[DUMMY_TASK_ID_0].timestampEnter = 0;
    notifications[DUMMY_TASK_ID_0].timestampExit  = 100;
    notifications[DUMMY_TASK_ID_0].duration       = 100;
    OS_GetTickCount_ExpectAndReturn(0u);
    SYSM_CheckNotifications();
}

void testSYSM_CheckNotificationsSYSMDisabled(void) {
    /* call a task with Monitoring disabled */
    OS_GetTickCount_ExpectAndReturn(0u);
    SYSM_CheckNotifications();

    OS_GetTickCount_ExpectAndReturn(100u);
    SYSM_CheckNotifications();
}

void testSYSM_CheckNotificationsProvokeDurationViolation(void) {
    OS_EnterTaskCritical_Ignore();
    OS_ExitTaskCritical_Ignore();

    /* provoke the violation of the task duration */
    OS_GetTickCount_ExpectAndReturn(0u);
    SYSM_CheckNotifications();

    SYSM_NOTIFICATION_s *notifications            = TEST_SYSM_GetNotifications();
    notifications[DUMMY_TASK_ID_0].timestampEnter = 0;
    notifications[DUMMY_TASK_ID_0].timestampExit  = 100;
    notifications[DUMMY_TASK_ID_0].duration       = 100;

    OS_GetTickCount_ExpectAndReturn(100u);
    DIAG_Handler_ExpectAndReturn(
        DIAG_ID_SYSTEM_MONITORING, DIAG_EVENT_NOT_OK, DIAG_SYSTEM, DUMMY_TASK_ID_0, DIAG_HANDLER_RETURN_OK);
    SYSM_CheckNotifications();
}

/** same test as #testSYSM_CheckNotificationsProvokeDurationViolation() but with recording enabled */
void testSYSM_CheckNotificationsProvokeDurationViolationWithRecording(void) {
    OS_EnterTaskCritical_Ignore();
    OS_ExitTaskCritical_Ignore();

    /* provoke the violation of the task duration */
    OS_GetTickCount_ExpectAndReturn(0u);
    SYSM_CheckNotifications();

    SYSM_NOTIFICATION_s *notifications            = TEST_SYSM_GetNotifications();
    notifications[DUMMY_TASK_ID_0].timestampEnter = 0;
    notifications[DUMMY_TASK_ID_0].timestampExit  = 100;
    notifications[DUMMY_TASK_ID_0].duration       = 100;

    OS_GetTickCount_ExpectAndReturn(100u);
    DIAG_Handler_ExpectAndReturn(
        DIAG_ID_SYSTEM_MONITORING, DIAG_EVENT_NOT_OK, DIAG_SYSTEM, DUMMY_TASK_ID_0, DIAG_HANDLER_RETURN_OK);
    SYSM_CheckNotifications();

    /* check if violation has been recorded */
    FRAM_WriteData_ExpectAndReturn(FRAM_BLOCK_ID_SYS_MON_RECORD, STD_OK);
    SYSM_UpdateFramData();
    TEST_ASSERT_EQUAL(true, fram_sys_mon_record.anyTimingIssueOccurred);
}

void testSYSM_NotifyInvalidTaskID(void) {
    /* give an invalid Task ID to Notify */
    TEST_ASSERT_FAIL_ASSERT(SYSM_Notify(SYSM_TASK_ID_MAX + 1u, SYSM_NOTIFY_ENTER, 424242));
    SYSM_NOTIFICATION_s *notifications = TEST_SYSM_GetNotifications();
    TEST_ASSERT_NOT_EQUAL(424242, notifications[SYSM_TASK_ID_MAX + 1u].timestampEnter);
}

void testSYSM_NotifyEnterTimestampProperlySet(void) {
    /* check whether Notify properly sets the entry timestamp */
    OS_EnterTaskCritical_Expect();
    OS_ExitTaskCritical_Expect();
    TEST_ASSERT_PASS_ASSERT(SYSM_Notify(DUMMY_TASK_ID_0, SYSM_NOTIFY_ENTER, UINT32_MAX));
    SYSM_NOTIFICATION_s *notifications = TEST_SYSM_GetNotifications();
    TEST_ASSERT_EQUAL(UINT32_MAX, notifications[DUMMY_TASK_ID_0].timestampEnter);

    OS_EnterTaskCritical_Expect();
    OS_ExitTaskCritical_Expect();
    TEST_ASSERT_PASS_ASSERT(SYSM_Notify(DUMMY_TASK_ID_0, SYSM_NOTIFY_ENTER, 0));
    notifications = TEST_SYSM_GetNotifications();
    TEST_ASSERT_EQUAL(0, notifications[DUMMY_TASK_ID_0].timestampEnter);
}

void testSYSM_NotifyExitTimestampProperlySetAndDurationCalculated(void) {
    /* check whether Notify properly sets the exit timestamp and computes duration */
    OS_EnterTaskCritical_Expect();
    OS_ExitTaskCritical_Expect();
    TEST_ASSERT_PASS_ASSERT(SYSM_Notify(DUMMY_TASK_ID_0, SYSM_NOTIFY_ENTER, UINT32_MAX));
    SYSM_NOTIFICATION_s *notifications = TEST_SYSM_GetNotifications();
    TEST_ASSERT_EQUAL(UINT32_MAX, notifications[DUMMY_TASK_ID_0].timestampEnter);

    const uint32 exitTime = 100;
    OS_EnterTaskCritical_Expect();
    OS_ExitTaskCritical_Expect();
    TEST_ASSERT_PASS_ASSERT(SYSM_Notify(DUMMY_TASK_ID_0, SYSM_NOTIFY_EXIT, exitTime));
    notifications = TEST_SYSM_GetNotifications();
    TEST_ASSERT_EQUAL(exitTime, notifications[DUMMY_TASK_ID_0].timestampExit);
    /* we go over the overflow, so it should be exit time plus one */
    TEST_ASSERT_EQUAL(exitTime + 1, notifications[DUMMY_TASK_ID_0].duration);
}

void testSYSM_NotifyHitAssertWithIllegalNotifyType(void) {
    /* This test hits the assert with an illegal notify type */
    OS_EnterTaskCritical_Ignore();
    /* use a notify type of INT8_MAX, as this is likely an illegal notify type */
    TEST_ASSERT_FAIL_ASSERT(SYSM_Notify(DUMMY_TASK_ID_0, INT8_MAX, UINT32_MAX));
    SYSM_NOTIFICATION_s *notifications = TEST_SYSM_GetNotifications();
    /* check that nothing has been written */
    TEST_ASSERT_NOT_EQUAL(UINT32_MAX, notifications[DUMMY_TASK_ID_0].timestampEnter);
    TEST_ASSERT_NOT_EQUAL(UINT32_MAX, notifications[DUMMY_TASK_ID_0].timestampExit);
}

/** test the edge cases of the function that can return recorded violations */
void testSYSM_GetRecordedTimingViolations(void) {
    OS_EnterTaskCritical_Ignore();
    OS_ExitTaskCritical_Ignore();

    /* default state is no violation, therefore no flag should be set */
    SYSM_TIMING_VIOLATION_RESPONSE_s violationResponse = {0};
    SYSM_GetRecordedTimingViolations(&violationResponse);
    TEST_ASSERT_FALSE(violationResponse.recordedViolationEngine);
    TEST_ASSERT_FALSE(violationResponse.recordedViolation1ms);
    TEST_ASSERT_FALSE(violationResponse.recordedViolation10ms);
    TEST_ASSERT_FALSE(violationResponse.recordedViolation100ms);
    TEST_ASSERT_FALSE(violationResponse.recordedViolation100msAlgo);

    /* when the general flag is set and one deviates in duration or entry, the violation should be set */
    fram_sys_mon_record.anyTimingIssueOccurred   = true;
    fram_sys_mon_record.taskEngineEnterTimestamp = 100u;
    fram_sys_mon_record.task1msViolatingDuration = 5u;
    SYSM_GetRecordedTimingViolations(&violationResponse);
    TEST_ASSERT_TRUE(violationResponse.recordedViolationAny);
    TEST_ASSERT_TRUE(violationResponse.recordedViolationEngine);
    TEST_ASSERT_TRUE(violationResponse.recordedViolation1ms);
    TEST_ASSERT_FALSE(violationResponse.recordedViolation10ms);
    TEST_ASSERT_FALSE(violationResponse.recordedViolation100ms);
    TEST_ASSERT_FALSE(violationResponse.recordedViolation100msAlgo);
}

/** test #SYSM_CopyFramStruct() and its reaction to invalid input */
void testSYSM_CopyFramStructInvalidInput(void) {
    FRAM_SYS_MON_RECORD_s dummy = {0};
    TEST_ASSERT_FAIL_ASSERT(SYSM_CopyFramStruct(NULL_PTR, &dummy));
    TEST_ASSERT_FAIL_ASSERT(SYSM_CopyFramStruct(&dummy, NULL_PTR));
}

/** test copy function of #SYSM_CopyFramStruct() */
void testSYSM_CopyFramStruct(void) {
    FRAM_SYS_MON_RECORD_s input = {
        .anyTimingIssueOccurred              = true,
        .task100msAlgorithmEnterTimestamp    = 1,
        .task100msAlgorithmViolatingDuration = 2,
        .task100msEnterTimestamp             = 3,
        .task100msViolatingDuration          = 4,
        .task10msEnterTimestamp              = 5,
        .task10msViolatingDuration           = 6,
        .task1msEnterTimestamp               = 7,
        .task1msViolatingDuration            = 8,
        .taskEngineEnterTimestamp            = 9,
        .taskEngineViolatingDuration         = 10,
    };
    FRAM_SYS_MON_RECORD_s output = {
        .anyTimingIssueOccurred              = false,
        .task100msAlgorithmEnterTimestamp    = 0,
        .task100msAlgorithmViolatingDuration = 0,
        .task100msEnterTimestamp             = 0,
        .task100msViolatingDuration          = 0,
        .task10msEnterTimestamp              = 0,
        .task10msViolatingDuration           = 0,
        .task1msEnterTimestamp               = 0,
        .task1msViolatingDuration            = 0,
        .taskEngineEnterTimestamp            = 0,
        .taskEngineViolatingDuration         = 0,
    };

    TEST_ASSERT_NOT_EQUAL(input.anyTimingIssueOccurred, output.anyTimingIssueOccurred);
    SYSM_CopyFramStruct(&input, &output);
    TEST_ASSERT_EQUAL(input.anyTimingIssueOccurred, output.anyTimingIssueOccurred);
    TEST_ASSERT_EQUAL(input.task100msAlgorithmEnterTimestamp, output.task100msAlgorithmEnterTimestamp);
    TEST_ASSERT_EQUAL(input.task100msAlgorithmViolatingDuration, output.task100msAlgorithmViolatingDuration);
    TEST_ASSERT_EQUAL(input.task100msEnterTimestamp, output.task100msEnterTimestamp);
    TEST_ASSERT_EQUAL(input.task100msViolatingDuration, output.task100msViolatingDuration);
    TEST_ASSERT_EQUAL(input.task10msEnterTimestamp, output.task10msEnterTimestamp);
    TEST_ASSERT_EQUAL(input.task10msViolatingDuration, output.task10msViolatingDuration);
    TEST_ASSERT_EQUAL(input.task1msEnterTimestamp, output.task1msEnterTimestamp);
    TEST_ASSERT_EQUAL(input.task1msViolatingDuration, output.task1msViolatingDuration);
    TEST_ASSERT_EQUAL(input.taskEngineEnterTimestamp, output.taskEngineEnterTimestamp);
    TEST_ASSERT_EQUAL(input.taskEngineViolatingDuration, output.taskEngineViolatingDuration);
}
