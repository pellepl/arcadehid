target remote localhost:3333

monitor reset halt

define hook-step
mon cortex_m maskisr on
end
define hookpost-step
mon cortex_m maskisr off
end
define hook-next
mon cortex_m maskisr on
end
define hookpost-next
mon cortex_m maskisr off
end
define hook-stepi
mon cortex_m maskisr on
end
define hookpost-stepi
mon cortex_m maskisr off
end

