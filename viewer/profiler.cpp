#include "profiler.h"
#include <QFile>
#include <QDomDocument>
#include <QDebug>
#include <cctype>
#include <profile/profile.hpp>
#include <profile/read.hpp>

namespace profiler
{
	call_ptr find_by_id(const profiler::calls& calls, call_id id)
	{
		for (auto&& c: calls)
			if (c->id() == id)
				return c;

		return nullptr;
	}

	void data::rebuild_profile(const profile::io::file_contents& file)
	{
		m_second = file.m_second;
		m_calls.clear();
		m_functions.clear();

		for (auto&& f: file.m_profile)
		{
			for (auto&& s: f)
			{
				QString name = QString::fromStdString(f.name());
				if (!s.name().empty())
					name.append("/").append(QString::fromStdString(s.name()));

				m_functions.push_back(std::make_shared<function>(s.id(), name, !s.name().empty()));

				for (auto&& c: s)
				{
					m_calls.push_back(std::make_shared<call>(c.id(), c.parent(), c.function(), c.duration(), c.flags()));
				}
			}
		}

		for (auto&& c: m_calls)
		{
			auto parent_id = c->parent();
			if (!parent_id)
				continue;

			auto parent = find_by_id(m_calls, c->parent());
			if (parent)
				parent->detract(c->duration());
		}

		qDebug() << "Got" << m_calls.size() << "calls and" << m_functions.size() << "functions.\n";
	}

	bool data::open(const QString &path)
	{
		profile::io::file_contents out;
		if (profile::io::read(path.toStdString().c_str(), out))
			return rebuild_profile(out), true;

		return false;
	}
}
