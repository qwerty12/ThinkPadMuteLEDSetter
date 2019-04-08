A terribly-written kext that calls on the Lenovo X250 ACPI method to set the laptop's mute state when the kext's mute state property is changed.

Fixer is a misnomer, this is simply a setter.

Having a properly synchronised mute state is useful because:

* it's nice not to have the led and the actual mute state out of sync (happens when the laptop is muted through other means than the physical mute button)

* when the mute state in the laptop's NVRAM is set to mute when shutting down, the BIOS/UEFI beep that happens when you press F2 to enter setup will not occur

I don't recommend using this so only the source is present.

You can tell the kext to set the state with RehabMan's ioio.

I use the following Hammerspoon config which works most of the time:

```
function ThinkPadMuteLEDFixerClient(mute)
	if mute then
		os.execute("/usr/local/bin/ioio -s ThinkPadMuteLEDFixer MuteStateNVRAM true >/dev/null 2>&1")
	else
		os.execute("/usr/local/bin/ioio -s ThinkPadMuteLEDFixer MuteStateNVRAM false >/dev/null 2>&1")
	end
end


function audioWatch(uid, event, scope, element)
	if (scope == "outp" or scope == "glob") and element == 0 then
		if event == "mute" then
			if dev:muted() then
				ThinkPadMuteLEDFixerClient(true)
			else
				ThinkPadMuteLEDFixerClient(false)
			end
		end
	end
end

dev = hs.audiodevice.findOutputByName("Built-in Output")
if not dev then
	dev = hs.audiodevice.defaultOutputDevice()
end
if dev then
	if dev:muted() then
		ThinkPadMuteLEDFixerClient(false)
		hs.timer.usleep(500000)
		ThinkPadMuteLEDFixerClient(true)
	else
		ThinkPadMuteLEDFixerClient(true)
		hs.timer.usleep(500000)
		ThinkPadMuteLEDFixerClient(false)
	end

	dev:watcherCallback(audioWatch)
	dev:watcherStart()
end
```
