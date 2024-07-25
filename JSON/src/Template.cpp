//
// Template.cpp
//
// Library: JSON
// Package: JSON
// Module:  Template
//
// Copyright (c) 2012, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/JSON/Template.h"
#include "Poco/JSON/TemplateCache.h"
#include "Poco/JSON/Query.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"


using Poco::Dynamic::Var;


namespace Poco {
namespace JSON {


POCO_IMPLEMENT_EXCEPTION(JSONTemplateException, Exception, "Template Exception")


class Part
{
public:
	Part() = default;

	virtual ~Part() = default;

	virtual void render(const Var& data, std::ostream& out) const = 0;

	using VectorParts = std::vector<SharedPtr<Part>>;
};


class StringPart: public Part
{
public:
	StringPart(): Part()
	{
	}

	StringPart(const std::string& content): Part(), _content(content)
	{
	}

	~StringPart() override = default;

	void render(const Var& data, std::ostream& out) const override
	{
		out << _content;
	}

	void setContent(const std::string& content)
	{
		_content = content;
	}

	inline std::string getContent() const
	{
		return _content;
	}

private:
	std::string _content;
};


class MultiPart: public Part
{
public:
	MultiPart() = default;

	~MultiPart() override = default;

	virtual void addPart(Part* part)
	{
		_parts.emplace_back(part);
	}

	void render(const Var& data, std::ostream& out) const override
	{
		for (const auto& p: _parts)
		{
			p->render(data, out);
		}
	}

protected:
	VectorParts _parts;
};


class EchoPart: public Part
{
public:
	EchoPart(const std::string& query): Part(), _query(query)
	{
	}

	~EchoPart() override = default;

	void render(const Var& data, std::ostream& out) const override
	{
		Query query(data);
		Var value = query.find(_query);

		if (!value.isEmpty())
		{
			out << value.convert<std::string>();
		}
	}

private:
	std::string _query;
};


class LogicQuery
{
public:
	LogicQuery(const std::string& query): _queryString(query)
	{
	}

	virtual ~LogicQuery() = default;

	virtual bool apply(const Var& data) const
	{
		bool logic = false;

		Query query(data);
		Var value = query.find(_queryString);

		if (!value.isEmpty()) // When empty, logic will be false
		{
			if (value.isString())
				// An empty string must result in false, otherwise true
				// Which is not the case when we convert to bool with Var
			{
				auto s = value.convert<std::string>();
				logic = !s.empty();
			}
			else
			{
				// All other values, try to convert to bool
				// An empty object or array will turn into false
				// all other values depend on the convert<> in Var
				logic = value.convert<bool>();
			}
		}

		return logic;
	}

protected:
	std::string _queryString;
};


class LogicExistQuery: public LogicQuery
{
public:
	LogicExistQuery(const std::string& query): LogicQuery(query)
	{
	}

	~LogicExistQuery() override = default;

	bool apply(const Var& data) const override
	{
		Query query(data);
		Var value = query.find(_queryString);

		return !value.isEmpty();
	}
};


class LogicElseQuery: public LogicQuery
{
public:
	LogicElseQuery(): LogicQuery("")
	{
	}

	~LogicElseQuery() override = default;

	bool apply(const Var& data) const override
	{
		return true;
	}
};


class LogicPart: public MultiPart
{
public:
	LogicPart(): MultiPart()
	{
	}

	~LogicPart() override = default;

	void addPart(LogicQuery* query, Part* part)
	{
		MultiPart::addPart(part);
		_queries.emplace_back(query);
	}

	void addPart(Part* part) override
	{
		MultiPart::addPart(part);
		_queries.push_back(new LogicElseQuery());
	}

	void render(const Var& data, std::ostream& out) const override
	{
		int count = 0;
		for (auto it = _queries.begin(); it != _queries.end(); ++it, ++count)
		{
			if ((*it)->apply(data) && _parts.size() > count)
			{
				_parts[count]->render(data, out);
				break;
			}
		}
	}

private:
	std::vector<SharedPtr<LogicQuery>> _queries;
};


class LoopPart: public MultiPart
{
public:
	LoopPart(const std::string& name, const std::string& query): MultiPart(), _name(name), _query(query)
	{
	}

	~LoopPart() override = default;

	void render(const Var& data, std::ostream& out) const override
	{
		Query query(data);

		if (data.type() == typeid(Object::Ptr))
		{
			Object::Ptr dataObject = data.extract<Object::Ptr>();
			Array::Ptr array = query.findArray(_query);
			if (!array.isNull())
			{
				for (int i = 0; i < array->size(); i++)
				{
					Var value = array->get(i);
					dataObject->set(_name, value);
					MultiPart::render(data, out);
				}
				dataObject->remove(_name);
			}
		}
	}

private:
	std::string _name;
	std::string _query;
};


class IncludePart: public Part
{
public:

	IncludePart(const Path& parentPath, const Path& path):
		Part(),
		_path(path)
	{
		// When the path is relative, try to make it absolute based
		// on the path of the parent template. When the file doesn't
		// exist, we keep it relative and hope that the cache can
		// resolve it.
		if (_path.isRelative())
		{
			Path templatePath(parentPath, _path);
			File templateFile(templatePath);
			if (templateFile.exists())
			{
				_path = templatePath;
			}
		}
	}

	~IncludePart() override = default;

	void render(const Var& data, std::ostream& out) const override
	{
		TemplateCache* cache = TemplateCache::instance();
		if (cache == nullptr)
		{
			Template tpl(_path);
			tpl.parse();
			tpl.render(data, out);
		}
		else
		{
			Template::Ptr tpl = cache->getTemplate(_path);
			tpl->render(data, out);
		}
	}

private:
	Path _path;
};


Template::Template(const Path& templatePath):
	_parts(nullptr),
	_currentPart(nullptr),
	_templatePath(templatePath)
{
}


Template::Template():
	_parts(nullptr),
	_currentPart(nullptr)
{
}


Template::~Template()
{
	delete _parts;
}


void Template::parse()
{
	File file(_templatePath);
	if (file.exists())
	{
		FileInputStream fis(_templatePath.toString());
		parse(fis);
	}
}


void Template::parse(std::istream& in)
{
	_parseTime.update();

	_parts = new MultiPart;
	_currentPart = _parts;

	while (in.good())
	{
		std::string text = readText(in); // Try to read text first
		if (text.length() > 0)
		{
			_currentPart->addPart(new StringPart(text));
		}

		if (in.bad())
			break; // Nothing to do anymore

		std::string command = readTemplateCommand(in);  // Try to read a template command
		if (command.empty())
		{
			break;
		}

		readWhiteSpace(in);

		if (command.compare("echo") == 0)
		{
			std::string query = readQuery(in);
			if (query.empty())
			{
				throw JSONTemplateException("Missing query in <? echo ?>");
			}
			_currentPart->addPart(new EchoPart(query));
		}
		else if (command.compare("for") == 0)
		{
			std::string loopVariable = readWord(in);
			if (loopVariable.empty())
			{
				throw JSONTemplateException("Missing variable in <? for ?> command");
			}
			readWhiteSpace(in);

			std::string query = readQuery(in);
			if (query.empty())
			{
				throw JSONTemplateException("Missing query in <? for ?> command");
			}

			_partStack.push(_currentPart);
			auto part = new LoopPart(loopVariable, query);
			_partStack.push(part);
			_currentPart->addPart(part);
			_currentPart = part;
		}
		else if (command.compare("else") == 0)
		{
			if (_partStack.empty())
			{
				throw JSONTemplateException("Unexpected <? else ?> found");
			}
			_currentPart = _partStack.top();
			auto lp = dynamic_cast<LogicPart*>(_currentPart);
			if (lp == nullptr)
			{
				throw JSONTemplateException("Missing <? if ?> or <? ifexist ?> for <? else ?>");
			}
			auto part = new MultiPart();
			lp->addPart(part);
			_currentPart = part;
		}
		else if (command.compare("elsif") == 0 || command.compare("elif") == 0)
		{
			std::string query = readQuery(in);
			if (query.empty())
			{
				throw JSONTemplateException("Missing query in <? " + command + " ?>");
			}

			if (_partStack.empty())
			{
				throw JSONTemplateException("Unexpected <? elsif / elif ?> found");
			}

			_currentPart = _partStack.top();
			auto lp = dynamic_cast<LogicPart*>(_currentPart);
			if (lp == nullptr)
			{
				throw JSONTemplateException("Missing <? if ?> or <? ifexist ?> for <? elsif / elif ?>");
			}
			auto part = new MultiPart();
			lp->addPart(new LogicQuery(query), part);
			_currentPart = part;
		}
		else if (command.compare("endfor") == 0)
		{
			if (_partStack.size() < 2)
			{
				throw JSONTemplateException("Unexpected <? endfor ?> found");
			}
			MultiPart* loopPart = _partStack.top();
			auto lp = dynamic_cast<LoopPart*>(loopPart);
			if (lp == nullptr)
			{
				throw JSONTemplateException("Missing <? for ?> command");
			}
			_partStack.pop();
			_currentPart = _partStack.top();
			_partStack.pop();
		}
		else if (command.compare("endif") == 0)
		{
			if (_partStack.size() < 2)
			{
				throw JSONTemplateException("Unexpected <? endif ?> found");
			}

			_currentPart = _partStack.top();
			auto lp = dynamic_cast<LogicPart*>(_currentPart);
			if (lp == nullptr)
			{
				throw JSONTemplateException("Missing <? if ?> or <? ifexist ?> for <? endif ?>");
			}

			_partStack.pop();
			_currentPart = _partStack.top();
			_partStack.pop();
		}
		else if (command.compare("if") == 0 || command.compare("ifexist") == 0)
		{
			std::string query = readQuery(in);
			if (query.empty())
			{
				throw JSONTemplateException("Missing query in <? " + command + " ?>");
			}
			_partStack.push(_currentPart);
			auto lp = new LogicPart();
			_partStack.push(lp);
			_currentPart->addPart(lp);
			_currentPart = new MultiPart();
			if (command.compare("ifexist") == 0)
			{
				lp->addPart(new LogicExistQuery(query), _currentPart);
			}
			else
			{
				lp->addPart(new LogicQuery(query), _currentPart);
			}
		}
		else if (command.compare("include") == 0)
		{
			readWhiteSpace(in);
			std::string filename = readString(in);
			if (filename.empty())
			{
				throw JSONTemplateException("Missing filename in <? include ?>");
			}
			else
			{
				Path resolvePath(_templatePath);
				resolvePath.makeParent();
				_currentPart->addPart(new IncludePart(resolvePath, filename));
			}
		}
		else
		{
			throw JSONTemplateException("Unknown command " + command);
		}

		readWhiteSpace(in);

		int c = in.get();
		if (c == '?' && in.peek() == '>')
		{
			in.get(); // forget '>'

			if (command.compare("echo") != 0)
			{
				if (in.peek() == '\r')
				{
					in.get();
				}
				if (in.peek() == '\n')
				{
					in.get();
				}
			}
		}
		else
		{
			throw JSONTemplateException("Missing ?>");
		}
	}
}


std::string Template::readText(std::istream& in)
{
	std::string text;
	int c = in.get();
	while (c != -1)
	{
		if (c == '<')
		{
			if (in.peek() == '?')
			{
				in.get(); // forget '?'
				break;
			}
		}
		text += static_cast<char>(c);

		c = in.get();
	}
	return text;
}


std::string Template::readTemplateCommand(std::istream& in)
{
	std::string command;

	readWhiteSpace(in);

	int c = in.get();
	while (c != -1)
	{
		if (Ascii::isSpace(c))
			break;

		if (c == '?' && in.peek() == '>')
		{
			in.putback(static_cast<char>(c));
			break;
		}

		if (c == '=' && command.length() == 0)
		{
			command = "echo";
			break;
		}

		command += static_cast<char>(c);

		c = in.get();
	}

	return command;
}


std::string Template::readWord(std::istream& in)
{
	std::string word;
	int c;
	while ((c = in.peek()) != -1 && !Ascii::isSpace(c))
	{
		in.get();
		word += static_cast<char>(c);
	}
	return word;
}


std::string Template::readQuery(std::istream& in)
{
	std::string word;
	int c;
	while ((c = in.get()) != -1)
	{
		if (c == '?' && in.peek() == '>')
		{
			in.putback(static_cast<char>(c));
			break;
		}

		if (Ascii::isSpace(c))
		{
			break;
		}
		word += static_cast<char>(c);
	}
	return word;
}


void Template::readWhiteSpace(std::istream& in)
{
	int c;
	while ((c = in.peek()) != -1 && Ascii::isSpace(c))
	{
		in.get();
	}
}


std::string Template::readString(std::istream& in)
{
	std::string str;

	int c = in.get();
	if (c == '"')
	{
		while ((c = in.get()) != -1 && c != '"')
		{
			str += static_cast<char>(c);
		}
	}
	return str;
}


void Template::render(const Var& data, std::ostream& out) const
{
	_parts->render(data, out);
}


} } // namespace Poco::JSON
