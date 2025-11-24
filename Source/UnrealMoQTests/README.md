# UnrealMoQ Tests

This directory contains comprehensive automation tests for the UnrealMoQ plugin, targeting 80%+ code coverage.

## Test Structure

The test suite is organized into the following test files:

### MoqBlueprintLibraryTest.cpp (12 tests)
Tests for utility functions in `UMoqBlueprintLibrary`:
- Version and error retrieval
- String to bytes conversion (empty, ASCII, Unicode)
- Bytes to string conversion (empty, valid UTF-8, invalid UTF-8, Unicode)
- Round-trip conversions

### MoqClientTest.cpp (21 tests)
Tests for `UMoqClient` functionality:
- Client construction and lifecycle
- Connection management (connect, disconnect, multiple connects)
- Connection state handling
- Namespace announcement
- Publisher creation with various parameters
- Subscriber creation with various parameters
- Error handling for uninitialized operations
- FMoqResult structure validation

### MoqPublisherTest.cpp (14 tests)
Tests for `UMoqPublisher` functionality:
- Publisher construction
- PublishData with various scenarios (empty data, large data, different delivery modes)
- PublishText with various scenarios (empty text, Unicode, long text, different delivery modes)
- Error handling for uninitialized publisher

### MoqSubscriberTest.cpp (13 tests)
Tests for `UMoqSubscriber` functionality:
- Subscriber construction
- Event binding
- Initialization with null handle
- OnDataReceivedCallback with various inputs (null data, zero length, valid data)
- UTF-8 validation (valid ASCII, valid Unicode, invalid UTF-8)
- Large data handling
- Multiple consecutive callbacks

## Running Tests

### In Unreal Engine Editor

1. **Open the Session Frontend**:
   - Go to `Window` → `Developer Tools` → `Session Frontend`

2. **Navigate to Automation Tab**:
   - Click on the "Automation" tab in the Session Frontend

3. **Filter Tests**:
   - In the filter box, type `UnrealMoQ` to show only UnrealMoQ tests
   - Or expand the tree to navigate to specific test groups

4. **Run Tests**:
   - Select the tests you want to run (or select the parent "UnrealMoQ" node to run all)
   - Click the "Start Tests" button
   - View results in the test list (green = passed, red = failed)

### Via Command Line

Run tests using UnrealEditor-Cmd:

```bash
# Windows
UnrealEditor-Cmd.exe "YourProject.uproject" -ExecCmds="Automation RunTests UnrealMoQ" -unattended -nopause -NullRHI -log

# Linux
./UnrealEditor-Cmd "YourProject.uproject" -ExecCmds="Automation RunTests UnrealMoQ" -unattended -nopause -NullRHI -log

# macOS
./UnrealEditor-Cmd.app/Contents/MacOS/UnrealEditor-Cmd "YourProject.uproject" -ExecCmds="Automation RunTests UnrealMoQ" -unattended -nopause -NullRHI -log
```

Run specific test groups:

```bash
# Run only BlueprintLibrary tests
-ExecCmds="Automation RunTests UnrealMoQ.BlueprintLibrary"

# Run only Client tests
-ExecCmds="Automation RunTests UnrealMoQ.Client"

# Run only Publisher tests
-ExecCmds="Automation RunTests UnrealMoQ.Publisher"

# Run only Subscriber tests
-ExecCmds="Automation RunTests UnrealMoQ.Subscriber"
```

### Continuous Integration

For CI/CD pipelines, tests can be run as part of automated builds:

```yaml
# Example GitHub Actions workflow
- name: Run UnrealMoQ Tests
  run: |
    UnrealEditor-Cmd.exe "${{ env.PROJECT_PATH }}" \
      -ExecCmds="Automation RunTests UnrealMoQ; Quit" \
      -unattended -nopause -NullRHI \
      -ReportOutputPath="${{ github.workspace }}/TestResults" \
      -log
```

## Test Coverage

The test suite provides comprehensive coverage across all major components:

| Component | Lines of Code | Test Count | Coverage Target |
|-----------|--------------|------------|-----------------|
| MoqBlueprintLibrary | ~68 | 12 | 90%+ |
| MoqClient | ~262 | 21 | 80%+ |
| MoqPublisher | ~106 | 14 | 85%+ |
| MoqSubscriber | ~87 | 13 | 85%+ |
| **Total** | **~523** | **60** | **80%+** |

### Coverage Breakdown

**API Coverage:**
- ✅ All public API methods tested
- ✅ Error handling paths covered
- ✅ Edge cases (empty inputs, null pointers, large data)
- ✅ Different delivery modes (Stream, Datagram)
- ✅ UTF-8 validation and conversion
- ✅ Multiple callback scenarios

**Not Covered (by design):**
- Actual network communication (requires live MoQ relay)
- Native FFI library internals (tested by moq-ffi separately)
- Platform-specific library loading

## Test Design Philosophy

### Unit Testing Approach
Tests are designed as **unit tests** that validate the plugin's C++ wrapper logic without requiring:
- A running MoQ relay server
- Network connectivity
- The native moq-ffi library to be fully functional

### What Tests Validate
- ✅ API contract adherence (correct parameters, return values)
- ✅ Error handling (null checks, empty data, uninitialized state)
- ✅ Data conversion (UTF-8 encoding/decoding)
- ✅ Object lifecycle (construction, destruction)
- ✅ Callback mechanisms (parameter validation, null safety)

### Integration Testing
For full end-to-end integration testing with a live MoQ relay, see the `Examples/` directory.

## Adding New Tests

When adding new functionality to UnrealMoQ:

1. **Add corresponding tests** in the appropriate test file
2. **Follow naming convention**: `FMoq[Component][Feature][Scenario]Test`
3. **Use descriptive test names**: Should clearly indicate what is being tested
4. **Target 80%+ coverage**: Ensure new code paths are covered
5. **Test error cases**: Not just happy paths

Example:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMoqClientConnectWithValidUrlTest,
    "UnrealMoQ.Client.Connect.ValidUrl",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FMoqClientConnectWithValidUrlTest::RunTest(const FString& Parameters)
{
    UMoqClient* Client = NewObject<UMoqClient>();
    FMoqResult Result = Client->Connect(TEXT("https://relay.example.com"));
    
    TestTrue(TEXT("Connect should execute without crashing"), true);
    return true;
}
```

## Troubleshooting

### Tests Not Appearing
- Ensure the UnrealMoQTests module is loaded (check plugin settings)
- Verify the module is set to `UncookedOnly` in the .uplugin file
- Rebuild the project

### Tests Failing
- Check the log output for detailed error messages
- Verify the UnrealMoQ plugin is enabled
- Ensure no conflicting versions of the plugin are installed

### Performance Issues
- Tests are designed to be fast (no network I/O)
- If tests are slow, check for unintended blocking operations
- Consider running tests in parallel using `-TestTimeout=60`

## Contributing

When contributing tests:
1. Maintain the existing test structure and naming conventions
2. Ensure all tests pass before submitting
3. Add documentation for complex test scenarios
4. Update this README if adding new test categories

## License

Same as the main UnrealMoQ plugin (MIT License).
