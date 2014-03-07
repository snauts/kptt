-------------------------------------------------------------------------------
-- Generate Sine Table
-------------------------------------------------------------------------------

local buffer = ""
print("static char sin_h[] = {")
for i = 0, 127, 1 do
	local angle = -2 * math.pi * i / 128
	local height = 150 + 30 * math.sin(angle)
	buffer = buffer .. math.floor(height) .. ","
	if (i % 8) == 7 then
		buffer = "    " .. buffer
		print(buffer)
		buffer = ""
	else
		buffer = buffer .. " "
	end
end
if (string.len(buffer) > 0) then
	print(buffer)
end
print("};\n")

-------------------------------------------------------------------------------
-- Generate Music Table
-------------------------------------------------------------------------------

-- C4 - 0x1125
-- D4 - 0x133F
-- E4 - 0x159A
-- F4 - 0x16E3
-- G4 - 0x1981
-- A4 - 0x1CD6
-- B4 - 0x205E

local noteLength = 12

local notes = {
	C = 0x1125,
	D = 0x133F,
	E = 0x159A,
	F = 0x16E3,
	G = 0x1981,
	A = 0x1CD6,
	B = 0x205E,
	P = 0x0000,
}

local music = {
	{"C",1.0}, {"C",1.0}, {"C",1.0}, {"C",1.0,""},
	{"C",1.0}, {"E",1.0}, {"G",1.0}, {"E",1.0,""},
	{"F",1.0}, {"D",1.0}, {"F",1.0}, {"D",1.0,""},
	{"C",2.0}, {"P",2.0,"\n"},

	{"C",1.0}, {"C",1.0}, {"C",1.0}, {"C",1.0,""},
	{"C",1.0}, {"E",1.0}, {"G",1.0}, {"E",1.0,""},
	{"F",1.0}, {"D",1.0}, {"F",1.0}, {"D",1.0,""},
	{"C",2.0}, {"P",2.0,"\n"},

	{"C",2.0}, {"C",2.0,""},
--	{"C",1.0}, {"G",1.0}, {"E",2.0,""}, -- original
	{"C",0.5}, {"G",0.5}, {"C",0.5}, {"G",0.5}, {"E",2.0,""},
	{"F",1.0}, {"D",1.0}, {"F",1.0}, {"D",1.0,""},
	{"C",2.0}, {"P",2.0,"\n"},

	{"C",2.0}, {"C",2.0,""},
--	{"C",1.0}, {"G",1.0}, {"E",2.0,""}, -- original
	{"C",0.5}, {"G",0.5}, {"C",0.5}, {"G",0.5}, {"E",2.0,""},
	{"F",1.0}, {"D",1.0}, {"F",1.0}, {"D",1.0,""},
	{"C",2.0}, {"P",1.0}, {"P",1.0,""},
}

local function hex(num)
	return string.format("0x%02x", num)
end

local function format_array(name, Formater)
	local buffer = "   "
	print("static unsigned char " .. name .. "[] = {")
	for i = 1, #music, 1 do
		local note = music[i]
		buffer = buffer .. " " .. hex(Formater(note)) .. ","
		if note[3] then
			print(buffer .. note[3])
			buffer = "   "
		end
	end
	print("};")
	print("")
end

local function FormatDuration(note)
	return noteLength * note[2]
end

local function FormatFrequencyLow(note)
	return bit32.band(notes[note[1]], 0xff)
end

local function FormatFrequencyHigh(note)
	return bit32.rshift(notes[note[1]], 8)
end

format_array("music_delay", FormatDuration)

format_array("sid_freq_lo", FormatFrequencyLow)

format_array("sid_freq_hi", FormatFrequencyHigh)

-------------------------------------------------------------------------------
-- Generate Half Circle Table
-------------------------------------------------------------------------------

local function DeltaX(angle)
	return math.floor(24 * math.cos(angle))
end

local function AbsY(angle)
	return 182 - math.floor(64 * math.sin(angle))
end

local function GetAngle(i)
	return math.pi * math.abs((i / 64.0) - 1.0)
end

local buffer_x = ""
local buffer_y = ""
for i = 0, 127, 1 do
	buffer_x = string.format("%s%3d, ", buffer_x, DeltaX(GetAngle(i)))
	buffer_y = buffer_y .. AbsY(GetAngle(i)) .. ", "
	if (i % 8) == 7 then
		buffer_x = buffer_x .. "\n"
		buffer_y = buffer_y .. "\n"
	end
end

print("signed char dance_x[] = {")
print(buffer_x)
print("};")

print("unsigned char dance_y[] = {")
print(buffer_y)
print("};")

-------------------------------------------------------------------------------
-- Generate Circular Table
-------------------------------------------------------------------------------

local function CircleX(angle)
	return math.floor(24 * math.cos(angle))
end

local function CircleY(angle)
	return math.floor(24 * math.sin(angle))
end

local function FullAngle(i)
	return 2 * math.pi * (i / 128.0)
end

local buffer_x = ""
local buffer_y = ""
for i = 0, 127, 1 do
	buffer_x = string.format("%s%3d, ", buffer_x, CircleX(FullAngle(i)))
	buffer_y = string.format("%s%3d, ", buffer_y, CircleY(FullAngle(i)))
	if (i % 8) == 7 then
		buffer_x = buffer_x .. "\n"
		buffer_y = buffer_y .. "\n"
	end
end

print("signed char circle_x[] = {")
print(buffer_x)
print("};")

print("unsigned char circle_y[] = {")
print(buffer_y)
print("};")
