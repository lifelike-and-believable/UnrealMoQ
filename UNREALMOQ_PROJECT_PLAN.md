# UnrealMoQ Project Plan

**Version:** 1.0  
**Created:** 2025-01-20  
**Status:** Active Development  

---

## Executive Summary

UnrealMoQ is an Unreal Engine 5.7+ plugin providing C++ and Blueprint APIs for connecting to MoQ (Media over QUIC) relay networks. The plugin wraps the `moq-ffi` Rust library (IETF Draft 07) and abstracts the complexities of WebTransport/QUIC connections, namespace announcements, track publishing, and track subscriptions.

This document defines the project goals, requirements, ideal functionality, current implementation state, gap analysis, and a test-first task breakdown for bringing the plugin to production quality.

---

## Table of Contents

1. [Goals and Requirements](#1-goals-and-requirements)
2. [High-Level Architecture](#2-high-level-architecture)
3. [Ideal API Design](#3-ideal-api-design)
4. [Current State Assessment](#4-current-state-assessment)
5. [Gap Analysis](#5-gap-analysis)
6. [Test-First Task Breakdown](#6-test-first-task-breakdown)
7. [moq-ffi Recommendations](#7-moq-ffi-recommendations)
8. [Testing Strategy](#8-testing-strategy)
9. [Timeline and Milestones](#9-timeline-and-milestones)
10. [Risks and Mitigations](#10-risks-and-mitigations)

---

## 1. Goals and Requirements

### 1.1 Project Goals

| Goal | Description |
|------|-------------|
| **G1: Simplicity** | Abstract away MoQ protocol complexity; developers should not need to understand WebTransport, QUIC, or draft specifications |
| **G2: Blueprint-First** | Every feature must be fully accessible from Blueprints without C++ knowledge |
| **G3: Production Quality** | Robust error handling, reconnection support, thread safety, and comprehensive testing |
| **G4: Minimal Footprint** | Thin wrapper over moq-ffi; no redundant abstractions or unnecessary dependencies |
| **G5: Cross-Platform** | Support Windows, Linux, and macOS with identical APIs |
| **G6: Unreal Native** | Follow Unreal Engine patterns (UObject lifecycle, delegates, async actions, automation tests) |

### 1.2 Functional Requirements

#### Core Connectivity
| ID | Requirement | Priority |
|----|-------------|----------|
| FR-1 | Connect to MoQ relay using WebTransport URL (https://) | P0 |
| FR-2 | Disconnect gracefully, cleaning up all resources | P0 |
| FR-3 | Report connection state changes via delegate/event | P0 |
| FR-4 | Query connection status synchronously | P0 |
| FR-5 | Automatic reconnection with configurable retry policy | P1 |
| FR-6 | Connection timeout configuration | P1 |

#### Publishing
| ID | Requirement | Priority |
|----|-------------|----------|
| FR-10 | Announce namespaces to relay | P0 |
| FR-11 | Create publishers for namespace/track pairs | P0 |
| FR-12 | Publish binary data with delivery mode (Stream/Datagram) | P0 |
| FR-13 | Publish text data with delivery mode | P0 |
| FR-14 | Publisher lifecycle events (created, destroyed, error) | P1 |
| FR-15 | Backpressure notification when send buffer is full | P2 |

#### Subscribing
| ID | Requirement | Priority |
|----|-------------|----------|
| FR-20 | Subscribe to namespace/track pairs | P0 |
| FR-21 | Receive binary data via delegate/event | P0 |
| FR-22 | Receive text data via delegate/event | P0 |
| FR-23 | Unsubscribe from tracks explicitly | P0 |
| FR-24 | Subscriber lifecycle events (subscribed, unsubscribed, error) | P1 |
| FR-25 | Track announcement notifications (new tracks available) | P1 |

#### Utilities
| ID | Requirement | Priority |
|----|-------------|----------|
| FR-30 | Query library version | P0 |
| FR-31 | Query last error message | P0 |
| FR-32 | Binary/string conversion utilities | P0 |
| FR-33 | JSON serialization helpers | P2 |

### 1.3 Non-Functional Requirements

| ID | Requirement | Target |
|----|-------------|--------|
| NFR-1 | All network callbacks marshalled to game thread | 100% |
| NFR-2 | No blocking calls on game thread > 1ms | Always |
| NFR-3 | Test coverage for public API | > 90% |
| NFR-4 | Integration test against Cloudflare relay | Pass |
| NFR-5 | Memory leak free (no UObject leaks on disconnect) | Always |
| NFR-6 | Blueprint node documentation | Complete |

---

## 2. High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Unreal Engine                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        Blueprint Layer                               │   │
│  │  ┌──────────────┐  ┌──────────────────┐  ┌────────────────────────┐ │   │
│  │  │ UMoqBlueprint│  │ Async Actions    │  │ UMoqAutomation        │ │   │
│  │  │ Library      │  │ (Connect, Sub)   │  │ BlueprintLibrary      │ │   │
│  │  └──────────────┘  └──────────────────┘  └────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        UObject API Layer                             │   │
│  │  ┌──────────────┐  ┌──────────────────┐  ┌────────────────────────┐ │   │
│  │  │ UMoqClient   │  │ UMoqPublisher    │  │ UMoqSubscriber         │ │   │
│  │  │              │  │                  │  │                        │ │   │
│  │  │ - Connect    │  │ - PublishData    │  │ - OnDataReceived       │ │   │
│  │  │ - Disconnect │  │ - PublishText    │  │ - OnTextReceived       │ │   │
│  │  │ - Announce   │  │                  │  │ - Unsubscribe          │ │   │
│  │  │ - Subscribe  │  │                  │  │                        │ │   │
│  │  └──────────────┘  └──────────────────┘  └────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        FFI Bridge Layer                              │   │
│  │  ┌──────────────────────────────────────────────────────────────┐   │   │
│  │  │ moq_ffi.h - C API                                             │   │   │
│  │  │ MoqClient*, MoqPublisher*, MoqSubscriber* (opaque pointers)   │   │   │
│  │  │ Callbacks: Connection, Data, Track                            │   │   │
│  │  └──────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          moq-ffi (Rust Library)                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ moq-rs → quinn (QUIC) → WebTransport → MoQ Relay                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Component Responsibilities

| Component | Responsibility |
|-----------|---------------|
| **UMoqClient** | Connection lifecycle, namespace announcements, factory for publishers/subscribers |
| **UMoqPublisher** | Data transmission on a single track |
| **UMoqSubscriber** | Data reception on a single track |
| **UMoqBlueprintLibrary** | Static utilities, factory functions for Blueprints |
| **Async Actions** | Blueprint-friendly async operations (connect with timeout, subscribe with retry) |
| **FFI Bridge** | Translates between Unreal types and C FFI types, marshals callbacks |

### 2.2 Threading Model

```
┌─────────────────────────────────────────────────────────────────┐
│                       Game Thread                               │
│  - All UObject method calls                                     │
│  - All delegate/event broadcasts                                │
│  - All Blueprint execution                                      │
└─────────────────────────────────────────────────────────────────┘
                              ▲
                              │ AsyncTask(ENamedThreads::GameThread)
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    moq-ffi Callback Thread                      │
│  - Connection state callbacks                                   │
│  - Data received callbacks                                      │
│  - Track announcement callbacks                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Ideal API Design

### 3.1 UMoqClient Ideal Interface

```cpp
UCLASS(BlueprintType)
class UNREALMOQ_API UMoqClient : public UObject
{
    GENERATED_BODY()

public:
    // === Connection Management ===
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    FMoqResult Connect(const FString& RelayUrl);
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    FMoqResult ConnectWithOptions(const FString& RelayUrl, const FMoqConnectionOptions& Options);
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    void Disconnect();
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    bool IsConnected() const;
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    EMoqConnectionState GetConnectionState() const;
    
    // === Namespace Management ===
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    FMoqResult AnnounceNamespace(const FString& Namespace);
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    FMoqResult UnannounceNamespace(const FString& Namespace);
    
    // === Publisher Factory ===
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    UMoqPublisher* CreatePublisher(
        const FString& Namespace, 
        const FString& TrackName, 
        EMoqDeliveryMode DeliveryMode);
    
    // === Subscriber Factory ===
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    UMoqSubscriber* Subscribe(const FString& Namespace, const FString& TrackName);
    
    // === Events ===
    
    UPROPERTY(BlueprintAssignable, Category = "MoQ")
    FMoqConnectionStateChangedDelegate OnConnectionStateChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "MoQ")
    FMoqTrackAnnouncedDelegate OnTrackAnnounced;
    
    UPROPERTY(BlueprintAssignable, Category = "MoQ")
    FMoqErrorDelegate OnError;
};

// Connection options struct
USTRUCT(BlueprintType)
struct FMoqConnectionOptions
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ConnectionTimeoutSeconds = 30.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAutoReconnect = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReconnectDelaySeconds = 5.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxReconnectAttempts = 5;
};
```

### 3.2 UMoqSubscriber Ideal Interface

```cpp
UCLASS(BlueprintType)
class UNREALMOQ_API UMoqSubscriber : public UObject
{
    GENERATED_BODY()

public:
    // === Subscription Management ===
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    void Unsubscribe();
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    bool IsSubscribed() const;
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    FString GetNamespace() const;
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    FString GetTrackName() const;
    
    // === Events ===
    
    UPROPERTY(BlueprintAssignable, Category = "MoQ")
    FMoqDataReceivedDelegate OnDataReceived;
    
    UPROPERTY(BlueprintAssignable, Category = "MoQ")
    FMoqTextReceivedDelegate OnTextReceived;
    
    UPROPERTY(BlueprintAssignable, Category = "MoQ")
    FMoqSubscriptionStateChangedDelegate OnSubscriptionStateChanged;
};
```

### 3.3 UMoqPublisher Ideal Interface

```cpp
UCLASS(BlueprintType)
class UNREALMOQ_API UMoqPublisher : public UObject
{
    GENERATED_BODY()

public:
    // === Publishing ===
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    FMoqResult PublishData(const TArray<uint8>& Data, EMoqDeliveryMode DeliveryMode);
    
    UFUNCTION(BlueprintCallable, Category = "MoQ")
    FMoqResult PublishText(const FString& Text, EMoqDeliveryMode DeliveryMode);
    
    // === State ===
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    bool IsValid() const;
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    FString GetNamespace() const;
    
    UFUNCTION(BlueprintPure, Category = "MoQ")
    FString GetTrackName() const;
    
    // === Events ===
    
    UPROPERTY(BlueprintAssignable, Category = "MoQ")
    FMoqPublisherErrorDelegate OnError;
};
```

---

## 4. Current State Assessment

### 4.1 Implemented Features

| Feature | Status | Notes |
|---------|--------|-------|
| Connect to relay | ✅ Complete | Using `moq_connect()` |
| Disconnect | ✅ Complete | Using `moq_disconnect()` |
| Connection state events | ✅ Complete | `OnConnectionStateChanged` delegate |
| IsConnected query | ✅ Complete | Using `moq_is_connected()` |
| Announce namespace | ✅ Complete | Using `moq_announce_namespace()` |
| Create publisher | ✅ Complete | Using `moq_create_publisher_ex()` |
| Publish data | ✅ Complete | Using `moq_publish_data()` |
| Publish text | ✅ Complete | Convenience wrapper |
| Subscribe | ✅ Complete | Using `moq_subscribe()` |
| Data received events | ✅ Complete | `OnDataReceived`, `OnTextReceived` |
| Async connect action | ✅ Complete | `UMoqConnectClientAsyncAction` |
| Subscribe with retry | ✅ Complete | `UMoqSubscribeWithRetryAsyncAction` |
| Blueprint library | ✅ Complete | Static utilities |
| Automation blueprint lib | ✅ Complete | Test helpers |

### 4.2 Test Coverage

| Test Suite | Test Count | Coverage |
|------------|------------|----------|
| MoqClientTests | 21 | Connection, state, announcements |
| MoqPublisherTests | 14 | Publish data/text, delivery modes |
| MoqSubscriberTests | 13 | Data reception, event binding |
| MoqBlueprintLibraryTests | 12 | Static utilities |
| MoqAutomationTests (Integration) | 8 | Cloudflare relay end-to-end |
| **Total** | **68** | Core workflows covered |

### 4.3 Code Quality

| Aspect | Status |
|--------|--------|
| Thread safety | ✅ Callbacks marshalled via `AsyncTask` |
| Memory management | ⚠️ Relies on BeginDestroy for cleanup |
| Error handling | ✅ `FMoqResult` pattern used consistently |
| Documentation | ✅ README, Integration Guide, API docs |
| Blueprint exposure | ✅ All major functions exposed |

---

## 5. Gap Analysis

### 5.1 Missing API Features

| Gap | Priority | Description | Ideal Solution |
|-----|----------|-------------|----------------|
| **GAP-1: No Unsubscribe()** | P0 | Subscribers can only unsubscribe via destruction | Add `UMoqSubscriber::Unsubscribe()` + track state |
| **GAP-2: OnTrackAnnounced not wired** | P1 | Delegate declared but never fired | Requires moq-ffi enhancement (see §7) |
| **GAP-3: No reconnection support** | P1 | Connection loss is permanent | Add `FMoqConnectionOptions` with auto-reconnect |
| **GAP-4: No connection timeout** | P1 | Connect() blocks until success/fail | Add timeout to async action, options struct |
| **GAP-5: No publisher lifecycle events** | P2 | No notification of publisher errors | Add `OnError` delegate to `UMoqPublisher` |
| **GAP-6: No unannounce namespace** | P2 | Cannot revoke namespace announcement | Add `UnannounceNamespace()` (requires moq-ffi) |

### 5.2 Test Coverage Gaps

| Gap | Priority | Description |
|-----|----------|-------------|
| **TGAP-1: No async action unit tests** | P0 | `UMoqConnectClientAsyncAction` and `UMoqSubscribeWithRetryAsyncAction` untested in isolation |
| **TGAP-2: No thread safety tests** | P1 | Multi-threaded callback delivery not stress-tested |
| **TGAP-3: No reconnection tests** | P1 | Reconnection logic (once implemented) needs testing |
| **TGAP-4: No memory leak validation** | P1 | UObject lifecycle not validated under stress |
| **TGAP-5: No Blueprint functional tests** | P2 | Only C++ automation tests exist |

### 5.3 Implementation Issues

| Issue | Priority | Description |
|-------|----------|-------------|
| **ISS-1: Subscriber cleanup only in BeginDestroy** | P1 | Should support explicit Unsubscribe() |
| **ISS-2: Publisher stores namespace/track but no accessors** | P2 | Add GetNamespace(), GetTrackName() |
| **ISS-3: No IsSubscribed() query** | P1 | Cannot check subscription state |
| **ISS-4: Static callback functions use global state** | P2 | Consider instance-based callbacks |

---

## 6. Test-First Task Breakdown

### Phase 1: Foundation (API Completeness)

#### Task 1.1: Add Unsubscribe() to UMoqSubscriber

**Type:** Implementation  
**Dependencies:** None  
**Priority:** P0  
**Estimated Effort:** 2-4 hours  

**Objective:**  
Add explicit `Unsubscribe()` method to `UMoqSubscriber` that cleans up the native subscriber handle and updates state.

**Test-First Approach:**
```cpp
// Write these tests FIRST in MoqSubscriberTests.cpp

// Test: Unsubscribe sets state to unsubscribed
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqSubscriberUnsubscribeTest, 
    "UnrealMoQ.Unit.Subscriber.Unsubscribe", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMoqSubscriberUnsubscribeTest::RunTest(const FString& Parameters)
{
    UMoqSubscriber* Subscriber = NewObject<UMoqSubscriber>();
    // Mock: set up subscriber with valid handle
    
    TestTrue("Initially subscribed", Subscriber->IsSubscribed());
    
    Subscriber->Unsubscribe();
    
    TestFalse("After Unsubscribe, not subscribed", Subscriber->IsSubscribed());
    return true;
}

// Test: Unsubscribe is idempotent (safe to call multiple times)
// Test: Unsubscribe cleans up native handle
// Test: OnSubscriptionStateChanged fires on Unsubscribe
```

**Success Criteria:**
- [ ] `Unsubscribe()` method exists and is BlueprintCallable
- [ ] `IsSubscribed()` method exists and is BlueprintPure
- [ ] Calling `Unsubscribe()` releases native handle via `moq_subscriber_destroy()`
- [ ] Calling `Unsubscribe()` multiple times is safe (idempotent)
- [ ] All new tests pass

**Files to Modify:**
- `Source/UnrealMoQ/Public/MoqSubscriber.h` - Add declarations
- `Source/UnrealMoQ/Private/MoqSubscriber.cpp` - Add implementations
- `Source/UnrealMoQTests/Private/MoqSubscriberTests.cpp` - Add tests

---

#### Task 1.2: Add Namespace/Track Accessors

**Type:** Implementation  
**Dependencies:** None  
**Priority:** P2  
**Estimated Effort:** 1-2 hours  

**Objective:**  
Add `GetNamespace()` and `GetTrackName()` accessors to `UMoqPublisher` and `UMoqSubscriber`.

**Test-First Approach:**
```cpp
// Test: Publisher returns correct namespace and track
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoqPublisherAccessorsTest, ...)
bool FMoqPublisherAccessorsTest::RunTest(...)
{
    // Setup publisher with known namespace/track
    TestEqual("Namespace matches", Publisher->GetNamespace(), TEXT("test-ns"));
    TestEqual("TrackName matches", Publisher->GetTrackName(), TEXT("test-track"));
    return true;
}
```

**Success Criteria:**
- [ ] `GetNamespace()` and `GetTrackName()` on both classes
- [ ] BlueprintPure for Blueprint access
- [ ] Tests verify correct values returned

**Files to Modify:**
- `Source/UnrealMoQ/Public/MoqPublisher.h`
- `Source/UnrealMoQ/Private/MoqPublisher.cpp`
- `Source/UnrealMoQ/Public/MoqSubscriber.h`
- `Source/UnrealMoQ/Private/MoqSubscriber.cpp`
- Test files

---

#### Task 1.3: Add Async Action Unit Tests

**Type:** Testing  
**Dependencies:** None  
**Priority:** P0  
**Estimated Effort:** 3-4 hours  

**Objective:**  
Write unit tests for `UMoqConnectClientAsyncAction` and `UMoqSubscribeWithRetryAsyncAction` to validate timeout, retry, and success paths.

**Test-First Approach:**
```cpp
// Tests for UMoqConnectClientAsyncAction:
// - Test: Success path fires OnConnected
// - Test: Timeout fires OnFailed after specified duration
// - Test: Connection failure fires OnFailed
// - Test: Cancellation during connection

// Tests for UMoqSubscribeWithRetryAsyncAction:
// - Test: Success on first attempt fires OnSuccess
// - Test: Success after retry fires OnSuccess
// - Test: Max retries exceeded fires OnFailed
// - Test: Correct subscriber returned on success
```

**Success Criteria:**
- [ ] 4+ tests for `UMoqConnectClientAsyncAction`
- [ ] 4+ tests for `UMoqSubscribeWithRetryAsyncAction`
- [ ] Tests cover success, failure, and edge cases
- [ ] All tests pass

**Files to Create:**
- `Source/UnrealMoQTests/Private/MoqAsyncActionTests.cpp`

---

### Phase 2: Robustness (Connection Reliability)

#### Task 2.1: Connection Options Struct

**Type:** Implementation  
**Dependencies:** Phase 1 complete  
**Priority:** P1  
**Estimated Effort:** 2-3 hours  

**Objective:**  
Create `FMoqConnectionOptions` struct with timeout and reconnection settings.

**Test-First Approach:**
```cpp
// Test: Default options have sensible values
// Test: Options can be serialized/deserialized (for config)
// Test: ConnectWithOptions accepts options struct
```

**Success Criteria:**
- [ ] `FMoqConnectionOptions` struct defined with BlueprintType
- [ ] `ConnectionTimeoutSeconds`, `bAutoReconnect`, `ReconnectDelaySeconds`, `MaxReconnectAttempts` properties
- [ ] Default values documented and sensible
- [ ] `ConnectWithOptions()` method on `UMoqClient`

**Files to Modify:**
- `Source/UnrealMoQ/Public/MoqTypes.h` - Add struct
- `Source/UnrealMoQ/Public/MoqClient.h` - Add method
- `Source/UnrealMoQ/Private/MoqClient.cpp` - Implement

---

#### Task 2.2: Auto-Reconnection Support

**Type:** Implementation  
**Dependencies:** Task 2.1  
**Priority:** P1  
**Estimated Effort:** 4-6 hours  

**Objective:**  
Implement automatic reconnection when connection is lost, using options from `FMoqConnectionOptions`.

**Test-First Approach:**
```cpp
// Test: With auto-reconnect enabled, reconnects after disconnect
// Test: Respects ReconnectDelaySeconds between attempts
// Test: Stops after MaxReconnectAttempts
// Test: OnConnectionStateChanged reports Reconnecting state
// Test: Can disable auto-reconnect during reconnection
```

**Success Criteria:**
- [ ] `EMoqConnectionState::Reconnecting` added to enum
- [ ] Reconnection timer uses `FTimerManager`
- [ ] Attempts counter tracks reconnection attempts
- [ ] Manual `Disconnect()` cancels reconnection
- [ ] All tests pass

**Files to Modify:**
- `Source/UnrealMoQ/Public/MoqTypes.h` - Add Reconnecting state
- `Source/UnrealMoQ/Private/MoqClient.cpp` - Implement reconnection logic
- Test files

---

#### Task 2.3: Connection Timeout Integration

**Type:** Implementation  
**Dependencies:** Task 2.1  
**Priority:** P1  
**Estimated Effort:** 2-3 hours  

**Objective:**  
Integrate `ConnectionTimeoutSeconds` from options into the connection flow.

**Success Criteria:**
- [ ] `Connect()` respects timeout from options
- [ ] Timeout fires `OnConnectionStateChanged` with `Failed` state
- [ ] Error message indicates timeout
- [ ] Tests verify timeout behavior

---

### Phase 3: Events & Lifecycle

#### Task 3.1: Publisher Error Events

**Type:** Implementation  
**Dependencies:** Phase 1 complete  
**Priority:** P2  
**Estimated Effort:** 2-3 hours  

**Objective:**  
Add `OnError` delegate to `UMoqPublisher` for publish failures.

**Test-First Approach:**
```cpp
// Test: OnError fires when publish fails
// Test: Error includes meaningful message
```

**Success Criteria:**
- [ ] `FMoqPublisherErrorDelegate` defined
- [ ] `OnError` delegate on `UMoqPublisher`
- [ ] Fires when `moq_publish_data()` returns error

---

#### Task 3.2: Subscription State Events

**Type:** Implementation  
**Dependencies:** Task 1.1  
**Priority:** P2  
**Estimated Effort:** 2-3 hours  

**Objective:**  
Add `OnSubscriptionStateChanged` delegate to `UMoqSubscriber`.

**Success Criteria:**
- [ ] `EMoqSubscriptionState` enum (Subscribing, Subscribed, Unsubscribed, Error)
- [ ] `FMoqSubscriptionStateChangedDelegate` defined
- [ ] `OnSubscriptionStateChanged` delegate on `UMoqSubscriber`
- [ ] Fires on successful subscription, unsubscription, and errors

---

### Phase 4: Documentation & Polish

#### Task 4.1: Update README with New APIs

**Type:** Documentation  
**Dependencies:** Phases 1-3  
**Priority:** P1  
**Estimated Effort:** 2 hours  

**Success Criteria:**
- [ ] README documents all new methods
- [ ] API reference table updated
- [ ] Code examples updated

---

#### Task 4.2: Update Integration Guide

**Type:** Documentation  
**Dependencies:** Phases 1-3  
**Priority:** P1  
**Estimated Effort:** 2 hours  

**Success Criteria:**
- [ ] Integration Guide includes reconnection examples
- [ ] Best practices section updated
- [ ] Troubleshooting section covers new error scenarios

---

#### Task 4.3: Blueprint Functional Tests

**Type:** Testing  
**Dependencies:** Phases 1-3  
**Priority:** P2  
**Estimated Effort:** 4-6 hours  

**Objective:**  
Create Blueprint-based functional tests using the automation framework.

**Success Criteria:**
- [ ] Blueprint test for connect/publish/subscribe workflow
- [ ] Blueprint test for reconnection
- [ ] Tests can run in automation

---

## 7. moq-ffi Recommendations

The following features are blocked or limited by the current moq-ffi API. These should be tracked as upstream enhancement requests or implemented directly if we control the fork.

### 7.1 Track Announcement Callback

**Issue:** `MoqTrackCallback` typedef exists in `moq_ffi.h` but there is no function to register it.

**Current State:**
```c
typedef void (*MoqTrackCallback)(const char* namespace_str, const char* track, void* user_data);
```

**Required Addition:**
```c
// Register callback for track announcements
void moq_set_track_callback(MoqClient* client, MoqTrackCallback callback, void* user_data);
```

**Impact:** Without this, `UMoqClient::OnTrackAnnounced` can never fire, preventing discovery of available tracks.

**Priority:** P1

---

### 7.2 Unsubscribe Function

**Issue:** No `moq_unsubscribe()` function exists; cleanup only via `moq_subscriber_destroy()`.

**Current State:**
```c
void moq_subscriber_destroy(MoqSubscriber* subscriber);
```

**Required Addition:**
```c
// Explicitly unsubscribe from track (can resubscribe later with same subscriber)
MoqResultCode moq_unsubscribe(MoqSubscriber* subscriber);
```

**Impact:** Currently `UMoqSubscriber::Unsubscribe()` must destroy the handle, preventing resubscription without creating a new subscriber.

**Priority:** P1 (workaround: destroy and recreate)

---

### 7.3 Connection Options

**Issue:** No way to pass connection options (timeout, TLS settings) to `moq_connect()`.

**Required Addition:**
```c
typedef struct {
    float connection_timeout_seconds;
    bool skip_server_verification; // for testing
} MoqConnectionOptions;

MoqResultCode moq_connect_with_options(
    MoqClient* client, 
    const char* url, 
    const MoqConnectionOptions* options,
    MoqConnectionCallback callback,
    void* user_data);
```

**Impact:** Cannot configure connection timeout at FFI level; must implement in UE layer with potential race conditions.

**Priority:** P2

---

### 7.4 Unannounce Namespace

**Issue:** No function to revoke namespace announcement.

**Required Addition:**
```c
MoqResultCode moq_unannounce_namespace(MoqClient* client, const char* namespace_str);
```

**Priority:** P3 (rare use case)

---

## 8. Testing Strategy

### 8.1 Test Pyramid

```
                    ┌─────────────┐
                    │  Manual E2E │  (Cloudflare relay, packaged build)
                    └─────────────┘
                   /               \
          ┌───────────────────────────┐
          │   Integration Tests       │  (Network tests against real relay)
          │   8 existing, expand to   │
          │   15+ with reconnection   │
          └───────────────────────────┘
         /                             \
    ┌───────────────────────────────────────┐
    │         Automation Tests              │  (Unreal Automation Framework)
    │   60+ existing, expand to 100+        │
    │   Fast, deterministic, mocked FFI     │
    └───────────────────────────────────────┘
```

### 8.2 Test Categories

| Category | Framework | Purpose | Run Frequency |
|----------|-----------|---------|---------------|
| Unit | Unreal Automation | Test individual class methods | Every build |
| Integration | Unreal Automation + Network | Test end-to-end against relay | Nightly / PR |
| Blueprint | Functional Test Framework | Test Blueprint workflows | Weekly / Release |
| Stress | Custom | Memory, threading, load | Release |

### 8.3 Running Tests

```powershell
# Unit tests (fast, no network)
./Build/Scripts/Run-AutomationTests.ps1 -TestFilter "UnrealMoQ.Unit"

# Integration tests (requires network, env var)
$env:MOQ_AUTOMATION_ENABLE_NETWORK = "1"
./Build/Scripts/Run-AutomationTests.ps1 -TestFilter "UnrealMoQ.Network"

# All tests
./Build/Scripts/Run-AutomationTests.ps1 -TestFilter "UnrealMoQ"
```

---

## 9. Timeline and Milestones

### Milestone 1: API Complete (Week 1-2)
- [ ] Task 1.1: Unsubscribe()
- [ ] Task 1.2: Accessors
- [ ] Task 1.3: Async action tests

**Deliverable:** All public API methods implemented and tested.

### Milestone 2: Robust Connections (Week 3-4)
- [ ] Task 2.1: Connection options
- [ ] Task 2.2: Auto-reconnection
- [ ] Task 2.3: Connection timeout

**Deliverable:** Production-ready connection handling.

### Milestone 3: Events Complete (Week 5)
- [ ] Task 3.1: Publisher error events
- [ ] Task 3.2: Subscription state events

**Deliverable:** Full event-driven API.

### Milestone 4: Documentation & Polish (Week 6)
- [ ] Task 4.1: README update
- [ ] Task 4.2: Integration Guide update
- [ ] Task 4.3: Blueprint functional tests

**Deliverable:** Complete documentation and test coverage.

---

## 10. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| moq-ffi lacks required features | Medium | High | File issues upstream; implement workarounds in UE layer |
| Cloudflare relay API changes | Low | High | Pin to known-working moq-rs version; monitor draft updates |
| Threading issues under load | Medium | Medium | Add stress tests; review callback marshalling |
| UObject lifecycle edge cases | Medium | Medium | Add explicit cleanup in Unsubscribe/Disconnect |
| Cross-platform build failures | Medium | Medium | CI/CD for all platforms; test matrix |

---

## Appendix A: Glossary

| Term | Definition |
|------|------------|
| **MoQ** | Media over QUIC - IETF protocol for real-time media transport |
| **Relay** | Server that routes MoQ messages between publishers and subscribers |
| **Namespace** | Grouping mechanism for tracks (like a topic or channel) |
| **Track** | Named data stream within a namespace |
| **Delivery Mode** | Stream (ordered, reliable) or Datagram (unordered, unreliable) |
| **Draft 07** | IETF specification version compatible with Cloudflare relay |
| **FFI** | Foreign Function Interface - C API for calling Rust code |

---

## Appendix B: File Structure

```
UnrealMoQ/
├── Source/
│   ├── UnrealMoQ/
│   │   ├── Public/
│   │   │   ├── MoqClient.h
│   │   │   ├── MoqPublisher.h
│   │   │   ├── MoqSubscriber.h
│   │   │   ├── MoqBlueprintLibrary.h
│   │   │   ├── MoqTypes.h
│   │   │   └── MoqAsyncActions.h
│   │   └── Private/
│   │       ├── MoqClient.cpp
│   │       ├── MoqPublisher.cpp
│   │       ├── MoqSubscriber.cpp
│   │       ├── MoqBlueprintLibrary.cpp
│   │       └── MoqAsyncActions.cpp
│   └── UnrealMoQTests/
│       └── Private/
│           ├── MoqClientTests.cpp
│           ├── MoqPublisherTests.cpp
│           ├── MoqSubscriberTests.cpp
│           ├── MoqBlueprintLibraryTests.cpp
│           ├── MoqAsyncActionTests.cpp      [NEW]
│           └── MoqAutomationTests.cpp
└── ThirdParty/
    └── moq-ffi/
        ├── include/moq_ffi.h
        └── lib/<Platform>/
```

---

*End of Project Plan*
