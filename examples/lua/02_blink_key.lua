print("lua script 2: blink and key control")

while true do
    if key_read() == 1 then
        led_toggle(1)
        delay_ms(120)
    else
        led_toggle(1)
        delay_ms(500)
    end
end
