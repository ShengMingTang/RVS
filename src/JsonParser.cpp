/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2018, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
Original authors:

Universite Libre de Bruxelles, Brussels, Belgium:
Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be
Daniele Bonatto, Daniele.Bonatto@ulb.ac.be
Arnaud Schenkel, arnaud.schenkel@ulb.ac.be

Koninklijke Philips N.V., Eindhoven, The Netherlands:
Bart Kroon, bart.kroon@philips.com
Bart Sonneveldt, bart.sonneveldt@philips.com
*/

#include "JsonParser.hpp"

#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace json
{
	void skipWhitespace(std::istream& stream);
	void matchCharacter(std::istream& stream, std::istream::int_type expected);
	void matchText(std::istream& stream, std::string const& text);
	std::shared_ptr<Value> readValue(std::istream& stream);

	struct Value
	{
		Value(Node::Type type) : type(type) {}

		virtual ~Value() {}

		Node::Type type;
	};

	struct String : public Value
	{
	public:
		String(std::istream& stream) : Value(Node::Type::string)
		{
			matchCharacter(stream, '"');
			auto ch = stream.get();

			while (ch != '"') {
				if (ch == '\\') {
					throw std::logic_error("JSON parser: string escaping not implemented");
				}
				value.push_back(static_cast<char>(ch));
				ch = stream.get();
			}
		}

		std::string value;
	};

	struct Number : public Value
	{
		Number(std::istream& stream) : Value(Node::Type::number)
		{
			stream >> value;
		}

		double value;
	};

	struct Object : public Value
	{
		Object(std::istream& stream) : Value(Node::Type::object)
		{
			matchCharacter(stream, '{');
			skipWhitespace(stream);

			while (stream.peek() != '}') {
				if (!value.empty()) {
					matchCharacter(stream, ',');
					skipWhitespace(stream);
				}

				auto key = String(stream);
				skipWhitespace(stream);
				matchCharacter(stream, ':');
				skipWhitespace(stream);
				value[key.value] = readValue(stream);
				skipWhitespace(stream);
			}

			stream.get();
		}

		std::map<std::string, std::shared_ptr<Value>> value;
	};

	struct Array : public Value
	{
		Array(std::istream& stream) : Value(Node::Type::array)
		{
			matchCharacter(stream, '[');
			skipWhitespace(stream);

			if (stream.peek() != ']') {
				value.push_back(readValue(stream));
				skipWhitespace(stream);

				while (stream.peek() == ',') {
					stream.get();
					value.push_back(readValue(stream));
					skipWhitespace(stream);
				}
			}

			matchCharacter(stream, ']');
		}

		std::vector<std::shared_ptr<Value>> value;
	};

	struct Bool : public Value
	{
	public:
		Bool(std::istream& stream) : Value(Node::Type::boolean)
		{
			value = stream.peek() == 't';
			matchText(stream, value ? "true" : "false");
		}

		bool value;
	};

	struct Null : public Value
	{
		Null(std::istream& stream) : Value(Node::Type::null)
		{
			matchText(stream, "null");
		}
	};

	Node Node::readFrom(std::istream& stream)
	{
		try {
			stream.exceptions(std::ios::badbit | std::ios::failbit);
			auto value = readValue(stream);
			skipWhitespace(stream);

			if (!stream.eof()) {
				auto ch = stream.get();
				std::ostringstream what;
				what << "Stray character " << static_cast<char>(ch) << " (0x" << std::ios::hex << ch << ")";
				throw std::runtime_error(what.str());
			}

			return{ value };
		}
		catch (std::runtime_error& e) {
			throw std::runtime_error(std::string("JSON parser: ") + e.what());
		}
		catch (std::exception& e) {
			throw std::logic_error(std::string("JSON parser bug: ") + e.what());
		}
	}

	Node::Type Node::type() const
	{
		return m_value->type;
	}

	Node Node::at(std::string key) const
	{
		return { dynamic_cast<Object&>(*m_value).value.at(key) };
	}

	Node Node::at(std::size_t index) const
	{
		return{ dynamic_cast<Array&>(*m_value).value.at(index) };
	}

	std::size_t Node::size() const
	{
		return dynamic_cast<Array&>(*m_value).value.size();
	}

	double Node::asDouble() const
	{
		return dynamic_cast<Number&>(*m_value).value;
	}

	std::string const& Node::asString() const
	{
		return dynamic_cast<String&>(*m_value).value;
	}

	bool Node::asBool() const
	{
		return dynamic_cast<Bool&>(*m_value).value;
	}

	Node::Node(std::shared_ptr<Value> value)
		: m_value(value)
	{}

	void skipWhitespace(std::istream& stream)
	{
		while (!stream.eof() && std::isspace(stream.peek())) {
			stream.get();
		}
	}

	void matchCharacter(std::istream& stream, std::istream::int_type expected)
	{
		auto actual = stream.get();

		if (actual != expected) {
			std::ostringstream what;
			what << "Expected '" << static_cast<char>(expected) << "' but found '" << static_cast<char>(actual) << "' (0x" << std::hex << actual << ")";
			throw std::runtime_error(what.str());
		}
	}

	void matchText(std::istream& stream, std::string const& text)
	{
		for (auto ch : text) {
			matchCharacter(stream, ch);
		}
	}

	std::shared_ptr<Value> readValue(std::istream& stream)
	{
		skipWhitespace(stream);
		auto ch = stream.peek();

		switch (ch) {
		case '{': return std::make_shared<Object>(stream);
		case '[': return std::make_shared<Array>(stream);
		case '"': return std::make_shared<String>(stream);
		case 't': return std::make_shared<Bool>(stream);
		case 'f': return std::make_shared<Bool>(stream);
		case 'n': return std::make_shared<Null>(stream);
		default: break;
		}

		if (ch == '-' || std::isdigit(ch)) {
			return std::make_shared<Number>(stream);
		}

		std::ostringstream what;
		what << "Invalid character " << static_cast<char>(ch) << " (0x" << std::ios::hex << ch << ")";
		throw std::runtime_error(what.str());
	}
}
