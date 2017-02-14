#include "interpret.h"

typedef std::vector<uint8_t>::const_iterator vec_it;

static bool interpret_general(std::ostream & os, vec_it cmd, vec_it chksum);
static bool interpret_ext(std::ostream & os, vec_it cmd, vec_it chksum);
static bool interpret_denon(std::ostream & os, vec_it cmd, vec_it chksum);
static std::ostream & print_time(std::ostream & os, vec_it it);
template <unsigned bytes>
static std::ostream & print_number(std::ostream & os, vec_it it);
static std::ostream & print_element(std::ostream & os, uint8_t code);

bool interpret(std::ostream & os, const std::vector<uint8_t> & data)
{
	if(data.size() < 5 || data[0] != 0xff)
		return false;

	vec_it lingo = data.begin() + 3;
	vec_it cmd = data.begin() + 4;
	vec_it chksum = data.end() - 1;

	if(data[2] == 0x00)
	{
		lingo += 2;
		cmd += 2;
	}

	switch(data[1])
	{
	case 0x55:
		switch(*lingo)
		{
		case 0x00:
			return interpret_general(os, cmd, chksum);
		case 0x04:
			return interpret_ext(os, cmd, chksum);
		}
		return false;

	case 0xa9:
		if(*lingo != 0x00)
			return false;
		else
			return interpret_denon(os, cmd, chksum);

	default:
		return false;
	}
}

static bool interpret_general(std::ostream & os, vec_it cmd, vec_it chksum)
{
	switch(*cmd) // 1st command byte
	{
	case 0x00:
		os << " Request identify";
		return true;
	case 0x01:
		os << " Identify:";
		dump_raw(os, cmd + 1, chksum);
		return true;
	case 0x02:
		os << " ACK: ";
		if(*(cmd+1) == 0x00)
			os << "Success";
		else
			os << "ERROR";
		dump_raw(os, cmd + 1, chksum);
		return true;
	case 0x03:
		os << " Request Remote UI Mode";
		return true;
	case 0x04:
		os << " Return Remote UI Mode: ";
		if(*(cmd+1) == 0)
			os << "Standard UI Mode";
		else
			os << "Extended Interface Mode";
		return true;
	case 0x05:
		os << " Enter Remote UI Mode";
		return true;
	case 0x06:
		os << " Exit Remote UI Mode";
		return true;
	case 0x09:
		os << " Request iPod Software Version";
		return true;
	case 0x0a:
		os << " iPod Software Version:";
		dump_raw(os, cmd + 1, chksum);
		return true;
	case 0x0b:
		os << " Request iPod Serial Number";
		return true;
	case 0x0c:
		os << " iPod Serial Number:";
		dump_utf8(os, cmd + 1);
		return true;
	case 0x0d:
		os << " Request iPod Model Number";
		return true;
	case 0x0e:
		os << " iPod Model Number:";
		dump_raw(os, cmd + 1, chksum);
		return true;
	case 0x0f:
		os << " Request Lingo Protocol Version";
		dump_raw(os, cmd + 1, chksum);
		return true;
	case 0x10:
		os << " Lingo Protocol Version:";
		dump_raw(os, cmd + 1, chksum);
		return true;
	default:
		return false;
	}
}

static bool interpret_ext(std::ostream & os, vec_it cmd, vec_it chksum)
{
	if(*cmd != 0x00)
		return false;

	++cmd;

	switch(*cmd)
	{
	case 0x01:
		os << " Result: ";
		if(*(cmd+1) == 0x00)
			os << "success";
		else
			os << "error (0x" << std::hex <<static_cast<unsigned>(*(cmd+1)) << ')';
		return true;

	case 0x12:
		os << " Request iPod type";
		return true;

	case 0x13:
		os << " iPod type:";
		dump_raw(os, cmd + 1, chksum);
		return true;

	case 0x16:
		os << " Switch to the main library playlist";
		return true;

	case 0x17:
		os << " Switch to ";
		print_element(os, *(cmd+1)) << ' ';
		print_number<4>(os, cmd + 2);
		return true;

	case 0x18:
		os << " Request count for element ";
		print_element(os, *(cmd+1));
		return true;

	case 0x19:
		os << " Count: ";
		print_number<4>(os, cmd + 1);
		return true;

	case 0x1a:
		os << " Request names for element ";
		print_element(os, *(cmd+1)) << "; starting at: ";
		print_number<4>(os, cmd + 2) << "; count: ";
		print_number<4>(os, cmd + 6);
		return true;

	case 0x1b:
		os << " Name for element ";
		print_number<4>(os, cmd + 1) << ": ";
		dump_utf8(os, cmd + 5);
		return true;

	case 0x1c:
		os << " Request time and status info";
		return true;

	case 0x1d:
		os << " Track length: ";
		print_time(os, cmd + 1) << "; current time: ";
		print_time(os, cmd + 5) << "; status: ";
		switch(*(cmd+9))
		{
		case 0x00: os << "stopped"; break;
		case 0x01: os << "playing"; break;
		case 0x02: os << "paused"; break;
		default: os << "? (0x" << std::hex <<static_cast<unsigned>(*(cmd+9)) << ')';
		}
		return true;

	case 0x1e:
		os << " Request current position in playlist";
		return true;

	case 0x1f:
		os << " Current position in playlist: ";
		print_number<4>(os, cmd + 1);
		return true;

	case 0x20:
		os << " Request title of song ";
		print_number<4>(os, cmd + 1);
		return true;

	case 0x21:
		os << " Title of song: ";
		dump_utf8(os, cmd + 1);
		return true;

	case 0x22:
		os << " Request artist of song ";
		print_number<4>(os, cmd + 1);
		return true;

	case 0x23:
		os << " Artist of song: ";
		dump_utf8(os, cmd + 1);
		return true;

	case 0x24:
		os << " Request album of song ";
		print_number<4>(os, cmd + 1);
		return true;

	case 0x25:
		os << " Album of song: ";
		dump_utf8(os, cmd + 1);
		return true;

	case 0x26:
		if(*(cmd+1) == 0x01)
			os << " Start polling mode";
		else if(*(cmd+1) == 0x00)
			os << " Stop polling mode";
		else
			os << " Polling mode: ? (0x" << std::hex << static_cast<unsigned>(*(cmd+1)) << ')';
		return true;

	case 0x27:
		os << " Time elapsed: ";
		// Byte *(cmd+1) ??
		print_time(os, cmd + 2);
		return true;

	case 0x28:
		os << " Switch to element ";
		if(*(cmd+1) == 0xff && *(cmd+2) == 0xff && *(cmd+3) == 0xff && *(cmd+4) == 0xff)
			os << "START";
		else
			print_number<4>(os, cmd + 1);
		return true;

	case 0x29:
		os << " Playback control: ";
		switch(*(cmd+1))
		{
		case 0x01: os << "Play/Pause"; break;
		case 0x02: os << "Stop"; break;
		case 0x03: os << "Skip++"; break;
		case 0x04: os << "Skip--"; break;
		case 0x05: os << "FFwd"; break;
		case 0x06: os << "FRwnd"; break;
		case 0x07: os << "Stop FFwd/FRwnd"; break;
		default: os << "? (0x" << std::hex << static_cast<unsigned>(*(cmd+1)) << ')';
		}
		return true;

	case 0x2c:
		os << " Get shuffle mode";
		return true;

	case 0x2d:
	case 0x2e:
		if(*cmd == 0x2d)
			os << " Shuffle mode: ";
		else
			os << " Set shuffle mode: ";
		switch(*(cmd+1))
		{
		case 0x00: os << "off"; break;
		case 0x01: os << "song"; break;
		case 0x02: os << "album"; break;
		default: os << "? (0x" << std::hex << static_cast<unsigned>(*(cmd+1)) << ')';
		}
		return true;

	case 0x2f:
		os << " Get repeat mode";
		return true;

	case 0x30:
	case 0x31:
		if(*cmd == 0x30)
			os << " Repeat mode: ";
		else
			os << " Set repeat mode: ";
		switch(*(cmd+1))
		{
		case 0x00: os << "off"; break;
		case 0x01: os << "one song"; break;
		case 0x02: os << "all songs"; break;
		default: os << "? (0x" << std::hex << static_cast<unsigned>(*(cmd+1)) << ')';
		}
		return true;

	case 0x32:
		os << " Picture block";
		return true;

	case 0x33:
		os << " Request screen resolution";
		return true;

	case 0x34:
		os << " Screen resolution: ";
		print_number<2>(os, cmd + 1) << 'x';
		print_number<2>(os, cmd + 3) << 'x';
		print_number<1>(os, cmd + 5);
		return true;

	default:
		return false;
	}
}

static bool interpret_denon(std::ostream & os, vec_it cmd, vec_it chksum)
{
	const std::vector<uint8_t> data(cmd, chksum);

	if(data == decltype(data){0x03})
		os << " Denon periodic status query";
	else if(data == decltype(data){0x81, 0x01, 0x01, 0x00, 0x00})
		os << " Denon status: no iPod";
	else if(data == decltype(data){0x81, 0x01, 0x00, 0x00, 0x00})
		os << " Denon status: busy #1";
	else if(data == decltype(data){0x81, 0x01, 0x01, 0x11, 0x00})
		os << " Denon status: busy #2";
	else if(data == decltype(data){0x81, 0x00, 0x01, 0x11, 0x00})
		os << " Denon status: iPod ready";
	else if(data == decltype(data){0x05})
		os << " Denon announcement: Deselecting iPod input";
	else if(data == decltype(data){0x80, 0x00, 0x05})
		os << " ACK: Deselecting iPod input";
	else if(data == decltype(data){0x01, 0x00})
		os << " Denon ??? #1";
	else if(data == decltype(data){0x80, 0x00, 0x01})
		os << " ACK: ??? #1";
	else
		return false;

	return true;
}

static std::ostream & print_time(std::ostream & os, vec_it it)
{
	uint32_t time = *it++;
	time = (time << 8) | *it++;
	time = (time << 8) | *it++;
	time = (time << 8) | *it++;

	const unsigned mins = time / 60 / 1000;
	const unsigned secs = time / 1000 - mins * 60;
	const unsigned msecs = time % 1000;

	os << std::setfill('0') << std::dec;
	return os << mins << ':' << std::setw(2) << secs << '.' << std::setw(3) << msecs;
}

template <unsigned bytes>
static std::ostream & print_number(std::ostream & os, vec_it it)
{
	unsigned num = 0;
	static_assert(sizeof(num) >= bytes, "ERROR");

	for(unsigned i = 0; i < bytes; ++i)
		num = (num << 8) | *it++;

	return os << std::dec << num;
}

static std::ostream & print_element(std::ostream & os, uint8_t code)
{
	switch(code)
	{
	case 0x01: return os << "Playlist";
	case 0x02: return os << "Artist";
	case 0x03: return os << "Album";
	case 0x04: return os << "Genre";
	case 0x05: return os << "Song";
	case 0x06: return os << "Composer";
	default: return os << "Unknown (0x" << std::hex << static_cast<unsigned>(code) << ')';
	}
}
