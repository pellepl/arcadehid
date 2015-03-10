arcadehid
=========

STM32 HID device for arcade builds

Compound HID device containing keyboard, mouse, two joysticks and a virtual com port. Plug into a retropie or something and wire cool buttons and joysticks and whatnot.

See /cad - here you'll find a reference design with 26 pins for an STM32F1 128kB flash variant. Any STM32F1 board would do, as long as pin-headers are broken out.

Each pin can be configured with combinations of a keyboard keypress, mouse movement or click, or joystick movement or button press.

Every pin can hold up to eight different combinations. There is also ternary support meaning a pin's config can change depending on if another pin is active or not.

Accelerators for mouse and joystick are supported.

Everything is configured in the command line interface, either via UART pins or via virtual com port.

Different configurations can be saved and loaded, using the internal flash of the STM32F1 (128 kb variant) - no extra storage chip needed.

Depends on pellepl/generic_embedded and pellepl/niffs.

Now also with the Annoyatron support, drive your friends mad...

