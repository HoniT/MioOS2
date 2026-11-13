## PS/2 Keyboard Driver

The PS/2 keyboard driver (`KeyboardDriver`) is a singleton device driver (`PS2DeviceDriver`, `util::Singleton<KeyboardDriver>`) that translates PS/2 Scan Code Set 2 interrupts into hardware-independent key events and ASCII characters.

---

### Initialization & Hardware Setup

Calling `KeyboardDriver::initialize()` performs the following sequence with interrupts disabled:
1. **Buffer Flush:** Drains residual bytes left in the PS/2 controller output buffer from device identification.
2. **Interrupt Routing:** Registers `KeyboardDriver::handle_interrupt_wrapper` on `KEYBOARD_INT_VECTOR` (`33`) and configures the corresponding I/O APIC redirection table entry (RTE).
3. **Device Configuration:** Sends `0xF4` (Enable Scanning) followed by `0xF0` + `0x02` to explicitly configure the keyboard to **Scan Code Set 2**.

---

### Scancode Processing & State Machine

On each IRQ (`handle_interrupt`), the driver reads a single byte from `PS2Controller::read_data()` and updates a multi-byte state machine:
* **`0xE0` (Extended Prefix):** Flags the upcoming byte as an extended key (e.g., arrow keys, Right Ctrl, Right Alt).
* **`0xF0` (Break Prefix):** Marks the next byte as a key-release (`is_press = false`) event.
* **Modifier Tracking:** Tracks active state for Shift, Ctrl, Alt, and toggles `CapsLock` on press.
* **ASCII Resolution:** On key press (`!is_release && !is_extended_scancode`), resolves printable characters via `lowercase_map` or `uppercase_map` using `shift_pressed ^ caps_lock_active`.
* **Event Queueing:** Pushes a `key_event_t` into an IRQ-safe queue guarded by `util::IrqSpinlock` (`queue_lock`), then resets the prefix flags.

---

### Public API & Data Structures

| Symbol | Description |
| :--- | :--- |
| `KeyCode` | `uint32_t` enum representing hardware-independent keys (`A`–`Z`, `Num0`–`Num9`, arrows, modifiers, punctuation, and function keys). |
| `KeyModifiers` | Bitmask flags (`MOD_NONE`, `MOD_SHIFT`, `MOD_CTRL`, `MOD_ALT`, `MOD_CAPS`) capturing modifier state at event time. |
| `key_event_t` | Event payload containing `KeyCode keycode`, `char ascii` (`0` if non-printable), `bool is_press`, and `uint8_t modifiers`. |
| `initialize()` | Configures the I/O APIC RTE, enables scanning, sets Scan Code Set 2, and returns `true` on success. |
| `get_curr_key_event()` | Pops and returns the oldest `key_event_t` from the event queue; returns `{KeyCode::Unknown, 0, false, MOD_NONE}` if empty. |

---

### Usage Example

~~~cpp

// Polling events from kernel/shell loop
key_event_t ev = kbd->get_curr_key_event();
if (ev.keycode != KeyCode::Unknown && ev.is_press) {
    if (ev.ascii != 0) {
        // Handle printable character
    }
}
~~~
