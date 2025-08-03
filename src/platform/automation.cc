// =================================================================================
// platform/automation.cc
// =================================================================================

#include "automation.hpp"

#include <comutil.h>  // for _variant_t, _bstr_t
#include <iostream>   // for std::wcout, std::wcerr
#include <memory>     // for std::addressof
#include <thread>     // for std::this_thread::sleep_for
#include <winerror.h> // for HRESULT codes like S_FALSE

namespace platform {

template <Com_Interface T>
Com_Pointer<T>::Com_Pointer() noexcept
    : pointer(nullptr)
{
}

template <Com_Interface T>
Com_Pointer<T>::Com_Pointer(T *const p) noexcept
    : pointer(p)
{
}

template <Com_Interface T>
Com_Pointer<T>::Com_Pointer(Com_Pointer &&other) noexcept
    : pointer(other.pointer)
{
    other.pointer = nullptr;
}

template <Com_Interface T>
auto Com_Pointer<T>::operator=(Com_Pointer &&other) noexcept -> Com_Pointer &
{
    if (this != std::addressof(other)) {
        if (pointer) pointer->Release();
        pointer = other.pointer;
        other.pointer = nullptr;
    }

    return *this;
}

template <Com_Interface T>
auto Com_Pointer<T>::operator->() noexcept -> T *
{
    return pointer;
}
template <Com_Interface T>
auto Com_Pointer<T>::operator->() const noexcept -> const T *
{
    return pointer;
}
template <Com_Interface T>
auto Com_Pointer<T>::get() const noexcept -> T *
{
    return pointer;
}
template <Com_Interface T>
Com_Pointer<T>::operator bool() const noexcept
{
    return pointer != nullptr;
}
template <Com_Interface T>
auto Com_Pointer<T>::operator&() noexcept -> T **
{
    if (pointer) {
        pointer->Release();
        pointer = nullptr;
    }
    return &pointer;
}

template <Com_Interface T>
auto Com_Pointer<T>::attach(T *const p) noexcept -> void
{
    if (pointer) pointer->Release();
    pointer = p;
}

template <Com_Interface T>
auto Com_Pointer<T>::detach() noexcept -> T *
{
    T *const p = pointer;
    pointer = nullptr;
    return p;
}

template <Com_Interface T>
auto Com_Pointer<T>::reset() noexcept -> void
{
    if (pointer) {
        pointer->Release();
        pointer = nullptr;
    }
}

template <Com_Interface T>
Com_Pointer<T>::~Com_Pointer()
{
    if (pointer) {
        pointer->Release();
        pointer = nullptr;
    }
}

auto send_string_via_keyboard(const std::wstring_view str) -> void
{
    std::vector<INPUT> inputs;
    inputs.reserve(str.length() * 2);

    for (const wchar_t c : str) {
        INPUT input_down = {};
        input_down.type = INPUT_KEYBOARD;
        input_down.ki.wVk = 0;
        input_down.ki.wScan = c;
        input_down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(input_down);

        INPUT input_up = input_down;
        input_up.ki.dwFlags |= KEYEVENTF_KEYUP;
        inputs.push_back(input_up);
    }

    if (!inputs.empty()) SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

auto set_value_fallback_via_keyboard(IUIAutomationElement *const element, const std::wstring_view value) -> HRESULT
{
    if (!element) return E_INVALIDARG;

    const HRESULT hr_focus = element->SetFocus();
    if (FAILED(hr_focus)) {
        std::wcerr << L"Warning: failed to set focus for keyboard fallback: HRESULT=" << std::hex << hr_focus << L'\n';
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    UIA_HWND hwnd_val;
    const HRESULT hr_hwnd = element->get_CurrentNativeWindowHandle(&hwnd_val);
    const HWND hwnd = reinterpret_cast<HWND>(hwnd_val);

    if (FAILED(hr_hwnd) || !hwnd) {
        std::wcerr << L"Error: couldn't get window handle for SendInput fallback\n";
        return E_FAIL;
    }

    SetForegroundWindow(hwnd);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // ctrl+a, delete
    std::array<INPUT, 4> inputs_clear = {{{INPUT_KEYBOARD, {0, VK_CONTROL, 0, 0, 0}},
                                          {INPUT_KEYBOARD, {0, 0x41 /*'A'*/, 0, 0, 0}},
                                          {INPUT_KEYBOARD, {0, VK_CONTROL, 0, KEYEVENTF_KEYUP, 0}},
                                          {INPUT_KEYBOARD, {0, VK_DELETE, 0, 0, 0}}}};

    SendInput(static_cast<UINT>(inputs_clear.size()), inputs_clear.data(), sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    send_string_via_keyboard(value);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    return S_OK;
}

Condition::Condition(IUIAutomationCondition *const p) noexcept
    : Com_Pointer(p)
{
}

Value_Pattern::Value_Pattern(IUIAutomationValuePattern *const p) noexcept
    : Com_Pointer(p)
{
}

auto Value_Pattern::set_value(const std::wstring_view text) const -> bool
{
    if (!pointer) return false;

    const _bstr_t bstr_text(std::wstring(text.begin(), text.end()).c_str());
    return SUCCEEDED(pointer->SetValue(bstr_text));
}

auto Value_Pattern::get_value() const -> std::optional<std::wstring>
{
    if (!pointer) return {};

    BSTR valueBSTR = nullptr;
    if (FAILED(pointer->get_CurrentValue(&valueBSTR)) || !valueBSTR) { return {}; }

    const std::wstring value_str(valueBSTR);
    SysFreeString(valueBSTR);
    return value_str;
}

Toggle_Pattern::Toggle_Pattern(IUIAutomationTogglePattern *const p) noexcept
    : Com_Pointer(p)
{
}

auto Toggle_Pattern::toggle() const -> bool
{
    if (!pointer) return false;
    return SUCCEEDED(pointer->Toggle());
}

auto Toggle_Pattern::get_toggle_state() const -> std::optional<ToggleState>
{
    if (!pointer) return {};

    ToggleState state;
    if (FAILED(pointer->get_CurrentToggleState(&state))) { return {}; }
    return state;
}

Invoke_Pattern::Invoke_Pattern(IUIAutomationInvokePattern *const p) noexcept
    : Com_Pointer(p)
{
}

auto Invoke_Pattern::invoke() const -> bool
{
    if (!pointer) return false;
    return SUCCEEDED(pointer->Invoke());
}

Element::Element(IUIAutomationElement *const element_ptr, IUIAutomation *const automation_ptr) noexcept
    : Com_Pointer(element_ptr)
    , automation_raw_ptr_(automation_ptr)
{
}

Element::Element(Element &&other) noexcept
    : Com_Pointer(std::move(other))
    , automation_raw_ptr_(other.automation_raw_ptr_)
{
    other.automation_raw_ptr_ = nullptr;
}

auto Element::operator=(Element &&other) noexcept -> Element &
{
    if (this != std::addressof(other)) {
        static_cast<Com_Pointer<IUIAutomationElement> &>(*this) = static_cast<Com_Pointer<IUIAutomationElement> &&>(other);
        automation_raw_ptr_ = other.automation_raw_ptr_;
        other.automation_raw_ptr_ = nullptr;
    }
    return *this;
}

auto Element::create_and_find_with_timeout(IUIAutomationElement *const search_root, const Condition &search_condition,
                                           const TreeScope scope, const std::chrono::seconds timeout) const -> std::optional<Element>
{
    if (!search_root || !automation_raw_ptr_ || !search_condition) return {};
    const auto end_time = std::chrono::high_resolution_clock::now() + timeout;

    IUIAutomationElement *found_raw_element = nullptr;
    while (std::chrono::high_resolution_clock::now() < end_time) {
        if (found_raw_element) {
            found_raw_element->Release();
            found_raw_element = nullptr;
        }

        const HRESULT hr_find = search_root->FindFirst(scope, search_condition.get(), &found_raw_element);
        if (SUCCEEDED(hr_find) && hr_find != S_FALSE && found_raw_element != nullptr) {
            return std::optional<Element>{std::in_place, found_raw_element, automation_raw_ptr_};
        }

        if (FAILED(hr_find) && hr_find != S_FALSE) {
            std::wcerr << L"Error: FindFirst (UIA search loop) failed: HRESULT=" << std::hex << hr_find << L'\n';
            return {};
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{150});
    }

    return {}; // timed out
}

auto Element::find_first(const TreeScope tree_scope, const Condition &condition) -> Element
{
    IUIAutomationElement *found_raw_element = nullptr;
    if (pointer) { std::ignore = pointer->FindFirst(tree_scope, condition.pointer, &found_raw_element); }
    return Element(found_raw_element, automation_raw_ptr_);
}

auto Element::set_focus() const -> bool
{
    if (!pointer) {
        std::wcerr << L"Error: element is null, cant set focus\n";
        return false;
    }

    const HRESULT hr = pointer->SetFocus();
    if (FAILED(hr)) {
        std::wcerr << L"Error: failed to set focus: HRESULT=" << std::hex << hr << L'\n';
        return false;
    }
    return true;
}

auto Element::get_native_window_handle() const -> HWND
{
    if (!pointer) {
        std::wcerr << L"Error: element is null, no window handle\n";
        return nullptr;
    }

    UIA_HWND hwnd_val;
    if (FAILED(pointer->get_CurrentNativeWindowHandle(&hwnd_val))) { return nullptr; }
    return reinterpret_cast<HWND>(hwnd_val);
}

auto Element::try_find_by_property_and_type(const std::wstring_view name_or_id, const PROPERTYID property_id,
                                            const Condition &type_condition, const std::chrono::seconds timeout) const
    -> std::optional<Element>
{

    IUIAutomationCondition *raw_prop_condition = nullptr;
    const _variant_t prop_variant(name_or_id.data());
    if (FAILED(automation_raw_ptr_->CreatePropertyCondition(property_id, prop_variant, &raw_prop_condition))) { return {}; }
    const Condition property_condition(raw_prop_condition);

    IUIAutomationCondition *raw_combined_condition = nullptr;
    if (FAILED(automation_raw_ptr_->CreateAndCondition(property_condition.get(), type_condition.get(), &raw_combined_condition))) {
        return {};
    }

    const Condition search_condition(raw_combined_condition);
    return create_and_find_with_timeout(pointer, search_condition, TreeScope_Descendants, timeout);
}

auto Element::try_find_by_automation_id_and_type(const std::wstring_view id_or_name, const Condition &type_condition,
                                                 const std::chrono::seconds timeout) const -> std::optional<Element>
{
    return try_find_by_property_and_type(id_or_name, UIA_AutomationIdPropertyId, type_condition, timeout);
}

auto Element::try_find_by_name_and_type(const std::wstring_view id_or_name, const Condition &type_condition,
                                        const std::chrono::seconds timeout) const -> std::optional<Element>
{
    return try_find_by_property_and_type(id_or_name, UIA_NamePropertyId, type_condition, timeout);
}

auto Element::find_element_by_id_or_name(const std::wstring_view id_or_name, std::chrono::seconds timeout) const -> std::optional<Element>
{
    if (!pointer || !automation_raw_ptr_) return {};

    // try finding by automation id first
    auto aid_condition = Automation().create_property_condition(UIA_AutomationIdPropertyId, id_or_name);
    if (aid_condition) {
        if (auto found = create_and_find_with_timeout(pointer, aid_condition, TreeScope_Descendants, timeout)) { return found; }
    }

    // if failed, try by name instead
    auto name_condition = Automation().create_property_condition(UIA_NamePropertyId, id_or_name);
    if (name_condition) {
        if (auto found = create_and_find_with_timeout(pointer, name_condition, TreeScope_Descendants, timeout)) { return found; }
    }

    return {};
}

auto Element::find_control_by_id_or_name_and_type(const std::wstring_view id_or_name, const CONTROLTYPEID control_type_id,
                                                  const std::chrono::seconds timeout) const -> std::optional<Element>
{

    if (!pointer || !automation_raw_ptr_) return {};

    const auto type_condition = Automation().create_control_type_condition(control_type_id);
    if (!type_condition) return {};

    if (auto found = try_find_by_automation_id_and_type(id_or_name, type_condition, timeout)) return found;
    if (auto found = try_find_by_name_and_type(id_or_name, type_condition, timeout)) return found;

    return {};
}

auto Element::find_input_field_by_name(const std::wstring_view id_or_name, const std::chrono::seconds timeout) const
    -> std::optional<Element>
{
    return find_control_by_id_or_name_and_type(id_or_name, UIA_EditControlTypeId, timeout);
}

auto Element::set_text(const std::wstring_view text) const -> bool
{
    if (!pointer) {
        std::wcerr << L"Error: element is null, cant set text\n";
        return false;
    }

    if (const auto value_pattern = get_value_pattern()) {
        if (value_pattern->set_value(text)) return true;
        std::wcerr << L"Warning: ValuePattern set_value failed, trying SendInput fallback...\n";
    } else {
        std::wcerr << L"Warning: element doesnt have ValuePattern, trying SendInput fallback...\n";
    }

    if (SUCCEEDED(set_value_fallback_via_keyboard(this->get(), text))) { return true; }

    std::wcerr << L"Error: SendInput fallback also failed.\n";
    return false;
}

auto Element::find_checkbox_by_name(const std::wstring_view id_or_name, const std::chrono::seconds timeout) const -> std::optional<Element>
{
    return find_control_by_id_or_name_and_type(id_or_name, UIA_CheckBoxControlTypeId, timeout);
}

auto Element::toggle_checkbox_by_name(const std::wstring_view id_or_name, const bool checked, const std::chrono::seconds timeout) const
    -> bool
{
    const auto checkbox_element = find_checkbox_by_name(id_or_name, timeout);
    if (!checkbox_element) return false;

    // not sure this is necessary
    checkbox_element->set_focus();

    const auto toggle_pattern = checkbox_element->get_toggle_pattern();
    if (!toggle_pattern) return false;

    const auto current_state = toggle_pattern->get_toggle_state();
    if (!current_state) return false;

    const bool should_toggle = (checked && *current_state == ToggleState_Off) || (!checked && *current_state == ToggleState_On);
    if (should_toggle) { return toggle_pattern->toggle(); }

    return true;
}

auto Element::find_button_by_name(const std::wstring_view id_or_name, const std::chrono::seconds timeout) const -> std::optional<Element>
{
    return find_control_by_id_or_name_and_type(id_or_name, UIA_ButtonControlTypeId, timeout);
}

auto Element::click() const -> bool
{
    if (const auto invoke_pattern = get_invoke_pattern()) {
        static_cast<void>(set_focus());
        return invoke_pattern->invoke();
    }

    std::wcerr << L"Error: element doesnt support InvokePattern (not clickable)\n";
    return false;
}

Automation::Automation()
{
    const HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pointer));
    if (FAILED(hr)) {
        std::wcerr << L"Error: failed to CoCreateInstance IUIAutomation: HRESULT=" << std::hex << hr << L'\n';
        pointer = nullptr;
    }
}

auto Automation::get_root_element() const -> Element
{
    IUIAutomationElement *raw_element = nullptr;
    if (pointer) { pointer->GetRootElement(&raw_element); }
    if (!raw_element) { std::wcerr << L"Error: failed to get root element\n"; }

    return Element(raw_element, const_cast<IUIAutomation *>(pointer));
}

auto Automation::get_element_from_handle(HWND handle) const -> std::optional<Element>
{
    if (!pointer) {
        std::wcerr << L"Error: Automation not initialized for get_element_from_handle\n";
        return {};
    }

    IUIAutomationElement *raw_element = nullptr;
    if (FAILED(pointer->ElementFromHandle(handle, &raw_element)) || !raw_element) { return {}; }

    return Element(raw_element, const_cast<IUIAutomation *>(pointer));
}

auto Automation::create_property_condition(PROPERTYID property_id, const std::wstring_view value) const -> Condition
{
    IUIAutomationCondition *raw_condition = nullptr;
    if (pointer) {
        const _variant_t var_value(std::wstring(value).c_str());
        pointer->CreatePropertyCondition(property_id, var_value, &raw_condition);
    }
    if (!raw_condition) { std::wcerr << L"Error: failed to create property condition for ID " << property_id << L'\n'; }
    return Condition(raw_condition);
}

auto Automation::create_control_type_condition(CONTROLTYPEID control_type_id) const -> Condition
{
    IUIAutomationCondition *raw_condition = nullptr;
    if (pointer) {
        _variant_t type_var;
        type_var.vt = VT_I4;
        type_var.lVal = control_type_id;

        pointer->CreatePropertyCondition(UIA_ControlTypePropertyId, type_var, &raw_condition);
    }

    if (!raw_condition) std::wcerr << L"Error: failed to create control type condition for ID " << control_type_id << L'\n';
    return Condition(raw_condition);
}

auto Automation::create_and_condition(const Condition &condition1, const Condition &condition2) const -> Condition
{
    IUIAutomationCondition *raw_and_condition = nullptr;
    if (pointer && condition1 && condition2) pointer->CreateAndCondition(condition1.get(), condition2.get(), &raw_and_condition);
    if (!raw_and_condition) std::wcerr << L"Error: failed to create AND condition\n";

    return Condition(raw_and_condition);
}

Co_Instance::Co_Instance()
{
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ok = (hr == S_OK || hr == S_FALSE);
    if (!ok) std::wcerr << L"Error: CoInitializeEx failed with HRESULT: " << std::hex << hr << L'\n';
}

Co_Instance::~Co_Instance()
{
    if (ok) CoUninitialize();
}

Co_Instance::operator bool() const
{
    return ok;
}

auto UIA_Operation_Error::print() const -> void
{
    std::wcerr << L"UIA Error: [" << static_cast<int>(code) << L"] " << message << L'\n';
}

UIA_Application::UIA_Application(const std::wstring_view window_name, const std::chrono::seconds timeout)
    : co_init_{}
    , automation_{}
{

    if (!co_init_) {
        set_error(UIA_Error_Code::COM_INIT_FAILED, L"COM init failed");
        return;
    }

    if (!automation_) {
        set_error(UIA_Error_Code::AUTOMATION_INIT_FAILED, L"UIA instance creation failed");
        return;
    }

    auto root = automation_.get_root_element();
    if (!root) {
        set_error(UIA_Error_Code::ROOT_ELEMENT_NOT_FOUND, L"couldn't get root UIA element");
        return;
    }

    const auto window_condition = automation_.create_property_condition(UIA_NamePropertyId, window_name);
    if (!window_condition) {
        set_error(UIA_Error_Code::INVALID_ARGUMENT, L"failed to create window search condition");
        return;
    }

    auto found_window = root.create_and_find_with_timeout(root.get(), window_condition, TreeScope_Children, timeout);
    if (!found_window) {
        const auto error_msg = L"target window '" + std::wstring(window_name) + L"' not found in time";
        set_error(UIA_Error_Code::TARGET_WINDOW_NOT_FOUND, error_msg);
        return;
    }

    target_window_.emplace(std::move(found_window.value()));
    std::wcout << L"UIA_Application: initialized and found target window '" << window_name << L"'\n";
}

auto UIA_Application::is_ready() const noexcept -> bool
{
    return target_window_.has_value();
}

auto UIA_Application::get_last_error() const noexcept -> std::optional<UIA_Operation_Error>
{
    return last_error_;
}

auto UIA_Application::set_text_in_field(const std::wstring_view field_name, const std::wstring_view text,
                                        const std::chrono::seconds timeout) -> UIA_Result<bool>
{
    if (!is_ready()) { return std::unexpected(UIA_Operation_Error{UIA_Error_Code::AUTOMATION_INIT_FAILED, L"UIA_Application not ready"}); }

    const auto field_element = target_window_->find_input_field_by_name(field_name, timeout);
    if (!field_element) {
        const auto error_msg = L"input field '" + std::wstring(field_name) + L"' not found";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::ELEMENT_NOT_FOUND, error_msg});
    }

    if (!field_element->set_text(text)) {
        const auto error_msg = L"failed to set text in field '" + std::wstring(field_name) + L"'";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::SET_VALUE_FAILED, error_msg});
    }

    return true;
}

auto UIA_Application::toggle_checkbox(const std::wstring_view checkbox_name, const bool checked, const std::chrono::seconds timeout)
    -> UIA_Result<bool>
{
    if (!is_ready()) { return std::unexpected(UIA_Operation_Error{UIA_Error_Code::AUTOMATION_INIT_FAILED, L"UIA_Application not ready"}); }

    if (!target_window_->toggle_checkbox_by_name(checkbox_name, checked, timeout)) {
        const auto error_msg = L"failed to toggle checkbox '" + std::wstring(checkbox_name) + L"'";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::TOGGLE_FAILED, error_msg});
    }

    return true;
}

auto UIA_Application::click_button(const std::wstring_view button_name, const std::chrono::seconds timeout) -> UIA_Result<bool>
{
    if (!is_ready()) { return std::unexpected(UIA_Operation_Error{UIA_Error_Code::AUTOMATION_INIT_FAILED, L"UIA_Application not ready"}); }

    const auto button_element = target_window_->find_button_by_name(button_name, timeout);
    if (!button_element) {
        const auto error_msg = L"button '" + std::wstring(button_name) + L"' not found";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::ELEMENT_NOT_FOUND, error_msg});
    }

    if (!button_element->click()) {
        const auto error_msg = L"failed to click button '" + std::wstring(button_name) + L"'";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::CLICK_FAILED, error_msg});
    }

    return true;
}

auto UIA_Application::set_focus_to_element(const std::wstring_view element_name, std::chrono::seconds timeout) -> UIA_Result<bool>
{
    if (!is_ready()) {
        const auto message = L"UIA_Application not ready";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::AUTOMATION_INIT_FAILED, message});
    }

    const auto element = target_window_->find_element_by_id_or_name(element_name, timeout);
    if (!element) {
        const auto error_msg = L"element '" + std::wstring(element_name) + L"' not found";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::ELEMENT_NOT_FOUND, error_msg});
    }

    if (!element->set_focus()) {
        const auto error_msg = L"failed to set focus on element '" + std::wstring(element_name) + L"'";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::FOCUS_FAILED, error_msg});
    }

    return true;
}

auto UIA_Application::send_key_to_window(const int virtual_key_code) -> UIA_Result<bool>
{
    if (!is_ready()) {
        const auto message = L"UIA_Application not ready";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::AUTOMATION_INIT_FAILED, message});
    }

    const HWND hwnd = target_window_->get_native_window_handle();
    if (!hwnd) {
        const auto message = L"couldn't get native window handle";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::NATIVE_WINDOW_HANDLE_NOT_FOUND, message});
    }

    SetForegroundWindow(hwnd);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    PostMessage(hwnd, WM_KEYDOWN, static_cast<WPARAM>(virtual_key_code), 0);
    PostMessage(hwnd, WM_KEYUP, static_cast<WPARAM>(virtual_key_code), 0);

    return true;
}

auto UIA_Application::send_string_to_window(const std::wstring_view text) -> UIA_Result<bool>
{
    if (!is_ready()) {
        const auto message = L"UIA_Application not ready";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::AUTOMATION_INIT_FAILED, message});
    }

    const HWND hwnd = target_window_->get_native_window_handle();
    if (!hwnd) {
        const auto message = L"couldn't get native window handle";
        return std::unexpected(UIA_Operation_Error{UIA_Error_Code::NATIVE_WINDOW_HANDLE_NOT_FOUND, message});
    }

    SetForegroundWindow(hwnd);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    send_string_via_keyboard(text);
    return true;
}

auto UIA_Application::set_error(const UIA_Error_Code code, const std::wstring_view message) -> void
{
    last_error_ = UIA_Operation_Error{code, std::wstring(message)};
    last_error_->print();
}

} // namespace platform
