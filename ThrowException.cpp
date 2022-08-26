#include "ThrowException.h"
#include <sstream>

ThrowException::ThrowException(int line, const char* file) noexcept
	:
	line(line),
	file(file)
{}

const char* ThrowException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< GetOriginString();
	whatbuffer = oss.str();
	return whatbuffer.c_str();
}

const char* ThrowException::GetType() const noexcept
{
	return "Exception Thrown!";
}

int ThrowException::GetLine() const noexcept
{
	return line;
}

const std::string& ThrowException::GetFile() const noexcept
{
	return file;
}

std::string ThrowException::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File]" << file << std::endl
		<< "[Line]" << line;
	return oss.str();
}