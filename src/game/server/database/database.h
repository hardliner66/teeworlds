#ifndef GAME_SERVER_DATABASE_DATABASE_H
#define GAME_SERVER_DATABASE_DATABASE_H

#include <string>
#include <iostream>

#include "sqlite/sqlite3.h"

class CDatabase
{
public:
	CDatabase() : m_db(nullptr) {}

	~CDatabase()
	{
		sqlite3_close(m_db);
	}

	int Open(const std::string& path);
	void AddBot(const std::string& username, const std::string& ip, const std::string& servername, const std::string& gamemode, const int version, const int flags, const bool vs_bots);
	bool IsBot(const std::string& username);
	bool DetectionTracked(const std::string& username, const std::string& ip, const std::string& servername, const std::string& gamemode, const int version, const int flags, const bool vs_bots);

	int user_version = 0;

private:
	sqlite3* m_db;
};

#endif// GAME_SERVER_DATABASE_DATABASE_H