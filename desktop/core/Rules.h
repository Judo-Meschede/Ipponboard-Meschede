// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#ifndef RULESET_H
#define RULESET_H

#include "Score.h"
#include <QString>
#include <QStringList>
#include <QMap>
#include <QByteArray>
#include <memory>

namespace Ipponboard
{
class Fight;
class AbstractRules
{
public:
	AbstractRules();

	virtual const char* Name() const = 0;

	virtual void SetAlwaysAutoAdjustPoints(bool autoAdjust)
	{
		_isAlwaysAutoAdjustPoints = autoAdjust;
	}

	virtual void SetCountSubscores(bool countSubscores)
	{
		_isCountSubscores = countSubscores;
	}

	virtual bool IsOption_AlwaysAutoAdjustPoints() const
	{
		return _isAlwaysAutoAdjustPoints;
	}

	virtual bool IsOption_CountSubscores() const
	{
		return _isCountSubscores;
	}

	virtual bool IsAwaseteIppon(Score const& s) const
	{
		return IsOption_AwaseteIppon() && s.Wazaari() == GetMaxWazaariCount();
	}

	// second shido will result in yuko beeing added, 3rd will give waza-ari
	virtual bool IsOption_ShidoAddsPoint() const { return false; }
	virtual bool IsOption_ShidoScoreCounts() const { return true; }
	virtual bool IsOption_AwaseteIppon() const { return true; }
	virtual bool IsOption_HasYuko() const { return true; }
	virtual bool IsOption_OpenEndGoldenScore() const { return true; }

	virtual int CompareScore(const Fight& f) const;
	virtual int GetMaxShidoCount() const { return 3; }
	virtual int GetMaxWazaariCount() const { return 2; }
	virtual int GetOsaekomiValue(Ipponboard::Score::Point p) const = 0;

	virtual int GetIpponTeamPoints() const { return 10; }
	virtual int GetWazaariTeamPoints() const { return 7; }
	virtual int GetYukoTeamPoints() const { return 5; }
	virtual int GetShidoTeamPoints() const { return 1; }

	virtual QString GetIpponLabel() const { return QStringLiteral("Ippon"); }
	virtual QString GetWazaariLabel() const { return QStringLiteral("Waza-ari"); }
	virtual QString GetYukoLabel() const { return QStringLiteral("Yuko"); }
	virtual QString GetShidoLabel() const { return QStringLiteral("Shido"); }
	virtual QString GetHansokumakeLabel() const { return QStringLiteral("Hansoku-make"); }
	virtual QString GetIpponShortLabel() const { return QStringLiteral("I"); }
	virtual QString GetWazaariShortLabel() const { return QStringLiteral("W"); }
	virtual QString GetYukoShortLabel() const { return QStringLiteral("Y"); }

	template<typename T>
	bool IsOfType() const { return dynamic_cast<const T*>(this) != nullptr; }

private:
	bool _isAlwaysAutoAdjustPoints { false };
	bool _isCountSubscores { false };
};

class ClassicRules : public AbstractRules
{
public:
	ClassicRules() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_OpenEndGoldenScore() const final { return false; }
	virtual bool IsOption_ShidoAddsPoint() const final { return true; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 25;

		case Score::Point::Wazaari: return 20;

		case Score::Point::Yuko: return 15;

		default: return -1;
		}
	}
};

class Rules2013 : public AbstractRules
{
public:
	Rules2013() {}

	static const char* const StaticName;

	virtual const char* Name() const final { return StaticName; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 15;

		case Score::Point::Yuko: return 10;

		default: return -1;
		}
	}
};

class Rules2017 : public AbstractRules
{
public:
	Rules2017() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return false; }
	virtual bool IsOption_AwaseteIppon() const { return false; }
	virtual bool IsAwaseteIppon(Score const&) const final { return false; }
	virtual int GetMaxShidoCount() const final { return 2; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return INT32_MAX; }
};

class Rules2017U15 : public AbstractRules
{
public:
	Rules2017U15() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return false; }
	virtual bool IsOption_AwaseteIppon() const { return false; }
	virtual bool IsAwaseteIppon(Score const&) const final { return false; }
	virtual int GetMaxShidoCount() const final { return 3; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return INT32_MAX; }
};

class Rules2018 : public AbstractRules
{
public:
	Rules2018() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return false; }
	virtual bool IsOption_AwaseteIppon() const { return true; }
	virtual int GetMaxShidoCount() const final { return 2; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return 2; }
};

class Rules2025 : public AbstractRules
{
public:
	Rules2025() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return true; }
	virtual bool IsOption_AwaseteIppon() const { return true; }
	virtual int GetMaxShidoCount() const final { return 2; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		case Score::Point::Yuko: return 5;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return 2; }
};

struct RulesDefinition
{
	QString name;
	bool hasYuko { true };
	bool awaseteIppon { true };
	bool openEndGoldenScore { true };
	bool shidoAddsPoint { false };
	bool shidoScoreCounts { false };
	int maxShidoCount { 2 };
	int maxWazaariCount { 2 };
	int osaekomiYukoSeconds { 5 };
	int osaekomiWazaariSeconds { 10 };
	int osaekomiIpponSeconds { 20 };
	int ipponTeamPoints { 10 };
	int wazaariTeamPoints { 7 };
	int yukoTeamPoints { 5 };
	int shidoTeamPoints { 1 };
	QString ipponLabel { QStringLiteral("Ippon") };
	QString wazaariLabel { QStringLiteral("Waza-ari") };
	QString yukoLabel { QStringLiteral("Yuko") };
	QString shidoLabel { QStringLiteral("Shido") };
	QString hansokumakeLabel { QStringLiteral("Hansoku-make") };
};

class ConfigurableRules : public AbstractRules
{
public:
	explicit ConfigurableRules(const RulesDefinition& definition)
		: m_definition(definition), m_name(definition.name.toUtf8()) {}

	const char* Name() const final { return m_name.constData(); }
	bool IsOption_ShidoAddsPoint() const final { return m_definition.shidoAddsPoint; }
	bool IsOption_ShidoScoreCounts() const final { return m_definition.shidoScoreCounts; }
	bool IsOption_AwaseteIppon() const final { return m_definition.awaseteIppon; }
	bool IsOption_HasYuko() const final { return m_definition.hasYuko; }
	bool IsOption_OpenEndGoldenScore() const final { return m_definition.openEndGoldenScore; }
	int GetMaxShidoCount() const final { return m_definition.maxShidoCount; }
	int GetMaxWazaariCount() const final { return m_definition.maxWazaariCount; }
	int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return m_definition.osaekomiIpponSeconds;
		case Score::Point::Wazaari: return m_definition.osaekomiWazaariSeconds;
		case Score::Point::Yuko: return m_definition.hasYuko ? m_definition.osaekomiYukoSeconds : -1;
		default: return -1;
		}
	}
	int GetIpponTeamPoints() const final { return m_definition.ipponTeamPoints; }
	int GetWazaariTeamPoints() const final { return m_definition.wazaariTeamPoints; }
	int GetYukoTeamPoints() const final { return m_definition.yukoTeamPoints; }
	int GetShidoTeamPoints() const final { return m_definition.shidoTeamPoints; }
	QString GetIpponLabel() const final { return m_definition.ipponLabel; }
	QString GetWazaariLabel() const final { return m_definition.wazaariLabel; }
	QString GetYukoLabel() const final { return m_definition.yukoLabel; }
	QString GetShidoLabel() const final { return m_definition.shidoLabel; }
	QString GetHansokumakeLabel() const final { return m_definition.hansokumakeLabel; }
	QString GetIpponShortLabel() const final { return m_definition.ipponLabel.left(1).toUpper(); }
	QString GetWazaariShortLabel() const final { return m_definition.wazaariLabel.left(1).toUpper(); }
	QString GetYukoShortLabel() const final { return m_definition.yukoLabel.left(1).toUpper(); }

private:
	RulesDefinition m_definition;
	QByteArray m_name;
};

class RulesFactory
{
private:
	static QMap<QString, RulesDefinition>& Definitions()
	{
		static QMap<QString, RulesDefinition> definitions;
		return definitions;
	}

public:
	static void ClearDefinitions() { Definitions().clear(); }
	static void RegisterDefinition(const RulesDefinition& definition)
	{
		if (!definition.name.isEmpty()) Definitions().insert(definition.name, definition);
	}
	static std::shared_ptr<AbstractRules> Create(QString name)
	{
		const auto dynamicRule = Definitions().constFind(name);
		if (dynamicRule != Definitions().constEnd())
			return std::make_shared<ConfigurableRules>(dynamicRule.value());
		if (name == ClassicRules::StaticName)
		{
			return std::make_shared<ClassicRules>();
		}

		if (name == Rules2013::StaticName)
		{
			return std::make_shared<Rules2013>();
		}

		if (name == Rules2017U15::StaticName)
		{
			return std::make_shared<Rules2017U15>();
		}

		if (name == Rules2017::StaticName)
		{
			return std::make_shared<Rules2017>();
		}

		if (name == Rules2018::StaticName)
		{
			return std::make_shared<Rules2018>();
		}

		if (name == Rules2025::StaticName)
		{
			return std::make_shared<Rules2025>();
		}

		// default
		return std::make_shared<Rules2025>();
	}

	static QStringList GetNames()
	{
		auto result = QStringList();
		for (auto it = Definitions().constBegin(); it != Definitions().constEnd(); ++it)
			result.push_back(it.key());

		result.push_back(Rules2025::StaticName);
		result.push_back(Rules2018::StaticName);
		result.push_back(Rules2017::StaticName);
		result.push_back(Rules2017U15::StaticName);
		result.push_back(Rules2013::StaticName);
		result.push_back(ClassicRules::StaticName);

		result.removeDuplicates();
		return result;
	}

	static QString GetDefaultName()
	{
		return Rules2025::StaticName;
	}
};
} // namespace
#endif // RULESET_H
