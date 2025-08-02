// =================================================================================
// platform/automation.hpp
// =================================================================================

#pragma once

#include <array>
#include <chrono>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <uiautomation.h>
#include <windows.h>

namespace platform {

template <class T>
concept Com_Interface = !std::is_pointer_v<T> && std::is_base_of_v<IUnknown, T>;

/**
 * @class Com_Pointer
 * @brief A move-only smart pointer for managing the lifetime of COM interfaces.
 */
template <Com_Interface T> struct Com_Pointer {
    T *pointer;

    Com_Pointer() noexcept;
    explicit Com_Pointer(T *p) noexcept;
    Com_Pointer(Com_Pointer &&other) noexcept;
    virtual ~Com_Pointer();

    Com_Pointer(const Com_Pointer &) = delete;
    auto operator=(const Com_Pointer &) -> Com_Pointer & = delete;

    auto operator=(Com_Pointer &&other) noexcept -> Com_Pointer &;
    auto operator->() noexcept -> T *;
    auto operator->() const noexcept -> const T *;
    explicit operator bool() const noexcept;
    auto operator&() noexcept -> T **;

    auto get() const noexcept -> T *;
    auto attach(T *p) noexcept -> void;
    auto detach() noexcept -> T *;
    auto reset() noexcept -> void;
};

struct Element;
struct Automation;

auto send_string_via_keyboard(const std::wstring_view str) -> void;
auto set_value_fallback_via_keyboard(IUIAutomationElement *const element, const std::wstring_view value) -> HRESULT;

/// @brief Wraps an IUIAutomationCondition interface used for finding elements.
struct Condition final : public Com_Pointer<IUIAutomationCondition> {
    explicit Condition(IUIAutomationCondition *p = nullptr) noexcept;
};

/// @brief Wraps an IUIAutomationValuePattern for controls that have a string value.
struct Value_Pattern final : public Com_Pointer<IUIAutomationValuePattern> {
    explicit Value_Pattern(IUIAutomationValuePattern *p = nullptr) noexcept;
    auto set_value(const std::wstring_view text) const -> bool;
    auto get_value() const -> std::optional<std::wstring>;
};

/// @brief Wraps an IUIAutomationTogglePattern for controls like checkboxes.
struct Toggle_Pattern final : public Com_Pointer<IUIAutomationTogglePattern> {
    explicit Toggle_Pattern(IUIAutomationTogglePattern *p = nullptr) noexcept;
    auto toggle() const -> bool;
    auto get_toggle_state() const -> std::optional<ToggleState>;
};

/// @brief Wraps an IUIAutomationInvokePattern for controls that can be invoked.
struct Invoke_Pattern final : public Com_Pointer<IUIAutomationInvokePattern> {
    explicit Invoke_Pattern(IUIAutomationInvokePattern *p = nullptr) noexcept;
    auto invoke() const -> bool;
};

/**
 * @class Element
 * @brief Represents a single UI element, providing methods for discovery and interaction.
 */
struct Element final : public Com_Pointer<IUIAutomationElement> {
    IUIAutomation *automation_raw_ptr_;

    explicit Element(IUIAutomationElement *element_ptr = nullptr, IUIAutomation *automation_ptr = nullptr) noexcept;
    Element(Element &&other) noexcept;
    auto operator=(Element &&other) noexcept -> Element &;
    Element(const Element &) = delete;
    auto operator=(const Element &) -> Element & = delete;

    [[nodiscard]] auto create_and_find_with_timeout(IUIAutomationElement *search_root, const Condition &search_condition, TreeScope scope,
                                                    std::chrono::seconds timeout) const -> std::optional<Element>;
    [[nodiscard]] auto find_first(TreeScope tree_scope, const Condition &condition) -> Element;
    [[nodiscard]] auto find_element_by_id_or_name(const std::wstring_view id_or_name, std::chrono::seconds timeout) const
        -> std::optional<Element>;
    [[nodiscard]] auto find_control_by_id_or_name_and_type(const std::wstring_view id_or_name, CONTROLTYPEID control_type_id,
                                                           std::chrono::seconds timeout) const -> std::optional<Element>;
    [[nodiscard]] auto find_input_field_by_name(const std::wstring_view id_or_name, std::chrono::seconds timeout) const
        -> std::optional<Element>;
    [[nodiscard]] auto find_checkbox_by_name(const std::wstring_view id_or_name, std::chrono::seconds timeout) const
        -> std::optional<Element>;
    [[nodiscard]] auto find_button_by_name(const std::wstring_view id_or_name, std::chrono::seconds timeout) const
        -> std::optional<Element>;

    /// @brief Gets the ValuePattern for this element, if supported.
    [[nodiscard]] auto get_value_pattern() const -> std::optional<Value_Pattern> {
        return get_pattern<Value_Pattern, IUIAutomationValuePattern>(UIA_ValuePatternId);
    }

    /// @brief Gets the TogglePattern for this element, if supported.
    [[nodiscard]] auto get_toggle_pattern() const -> std::optional<Toggle_Pattern> {
        return get_pattern<Toggle_Pattern, IUIAutomationTogglePattern>(UIA_TogglePatternId);
    }

    /// @brief Gets the InvokePattern for this element, if supported.
    [[nodiscard]] auto get_invoke_pattern() const -> std::optional<Invoke_Pattern> {
        return get_pattern<Invoke_Pattern, IUIAutomationInvokePattern>(UIA_InvokePatternId);
    }

    auto set_focus() const -> bool;
    [[nodiscard]] auto get_native_window_handle() const -> HWND;
    auto set_text(const std::wstring_view text) const -> bool;
    auto toggle_checkbox_by_name(const std::wstring_view id_or_name, const bool checked, std::chrono::seconds timeout) const -> bool;
    auto click() const -> bool;

  private:
    template <typename PatternWrapper, typename RawPatternInterface>
    [[nodiscard]] auto get_pattern(PATTERNID pattern_id) const -> std::optional<PatternWrapper> {
        if (!pointer) return {};

        RawPatternInterface *raw_pattern = nullptr;
        if (SUCCEEDED(pointer->GetCurrentPattern(pattern_id, reinterpret_cast<IUnknown **>(&raw_pattern))) && raw_pattern) {
            return PatternWrapper(raw_pattern);
        }
        return {};
    }

    [[nodiscard]] auto try_find_by_property_and_type(const std::wstring_view name_or_id, PROPERTYID property_id,
                                                     const Condition &type_condition, const std::chrono::seconds timeout) const
        -> std::optional<Element>;
    [[nodiscard]] auto try_find_by_automation_id_and_type(const std::wstring_view id_or_name, const Condition &type_condition,
                                                          std::chrono::seconds timeout) const -> std::optional<Element>;
    [[nodiscard]] auto try_find_by_name_and_type(const std::wstring_view id_or_name, const Condition &type_condition,
                                                 std::chrono::seconds timeout) const -> std::optional<Element>;
};

/**
 * @class Automation
 * @brief The root object for UIA operations, used to access the element tree.
 */
struct Automation final : public Com_Pointer<IUIAutomation> {
    Automation();
    [[nodiscard]] auto get_root_element() const -> Element;
    [[nodiscard]] auto get_element_from_handle(HWND handle) const -> std::optional<Element>;
    [[nodiscard]] auto create_property_condition(PROPERTYID property_id, const std::wstring_view value) const -> Condition;
    [[nodiscard]] auto create_control_type_condition(CONTROLTYPEID control_type_id) const -> Condition;
    [[nodiscard]] auto create_and_condition(const Condition &condition1, const Condition &condition2) const -> Condition;
};

/**
 * @class Co_Instance
 * @brief An RAII wrapper for COM library initialization on a thread.
 */
struct Co_Instance {
    bool ok = false;
    explicit operator bool() const;
    Co_Instance();
    ~Co_Instance();
};

/// @brief Categorized error codes for UIA operations.
enum class UIA_Error_Code {
    COM_INIT_FAILED,
    AUTOMATION_INIT_FAILED,
    ROOT_ELEMENT_NOT_FOUND,
    TARGET_WINDOW_NOT_FOUND,
    ELEMENT_NOT_FOUND,
    PATTERN_NOT_SUPPORTED,
    SET_VALUE_FAILED,
    TOGGLE_FAILED,
    CLICK_FAILED,
    FOCUS_FAILED,
    NATIVE_WINDOW_HANDLE_NOT_FOUND,
    UNKNOWN_ERROR,
    INVALID_ARGUMENT
};

/// @brief Contains an error code and a descriptive message for a failed UIA operation.
struct UIA_Operation_Error {
    UIA_Error_Code code;
    std::wstring message;
    auto print() const -> void;
};

/// @brief A result type for UIA operations.
template <typename T> using UIA_Result = std::expected<T, UIA_Operation_Error>;

/**
 * @class UIA_Application
 * @brief A high-level wrapper to find and automate a specific application window.
 */
class UIA_Application {
  public:
    [[nodiscard]] explicit UIA_Application(const std::wstring_view window_name, std::chrono::seconds timeout = std::chrono::seconds(20));

    /// @brief Returns true if the object is initialized and attached to the target window.
    [[nodiscard]] auto is_ready() const noexcept -> bool;

    /// @brief Returns the last error that occurred, if any.
    [[nodiscard]] auto get_last_error() const noexcept -> std::optional<UIA_Operation_Error>;

    /// @brief Finds an input field and sets its text.
    auto set_text_in_field(const std::wstring_view field_name, const std::wstring_view text,
                           std::chrono::seconds timeout = std::chrono::seconds(10)) -> UIA_Result<bool>;

    /// @brief Finds a checkbox and sets its toggle state.
    auto toggle_checkbox(const std::wstring_view checkbox_name, bool checked, std::chrono::seconds timeout = std::chrono::seconds(10))
        -> UIA_Result<bool>;

    /// @brief Finds a button and clicks it.
    auto click_button(const std::wstring_view button_name, std::chrono::seconds timeout = std::chrono::seconds(10)) -> UIA_Result<bool>;

    /// @brief Finds an element by name or ID and sets focus to it.
    auto set_focus_to_element(const std::wstring_view element_name, std::chrono::seconds timeout = std::chrono::seconds(10))
        -> UIA_Result<bool>;

    /// @brief Sends a virtual key press to the target window.
    auto send_key_to_window(int virtual_key_code) -> UIA_Result<bool>;

    /// @brief Sends a string to the target window via keyboard simulation.
    auto send_string_to_window(const std::wstring_view text) -> UIA_Result<bool>;

  private:
    Co_Instance co_init_;
    Automation automation_;
    std::optional<Element> target_window_;
    std::optional<UIA_Operation_Error> last_error_;

    auto set_error(UIA_Error_Code code, const std::wstring_view message) -> void;
};

} // namespace platform
