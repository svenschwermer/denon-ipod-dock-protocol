#ifndef INTERPRET_H_
#define INTERPRET_H_

#include <cstdint>
#include <vector>
#include <ostream>
#include <iomanip>

bool interpret(std::ostream & os, const std::vector<uint8_t> & data);

inline std::ostream & dump_raw(std::ostream & os, std::vector<uint8_t>::const_iterator begin,
		std::vector<uint8_t>::const_iterator end)
{
	while(begin != end)
		os << " 0x" << std::setw(2) << static_cast<unsigned>(*begin++);
	return os;
}

inline std::ostream & dump_utf8(std::ostream & os, std::vector<uint8_t>::const_iterator it)
{
	std::string str;
	char c;
	while((c = static_cast<char>(*it++)) != '\0')
		str.push_back(c);
	return os << str;
}

#endif /* INTERPRET_H_ */
