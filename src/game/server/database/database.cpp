#include "database.h"

#include <ctime>

// #define DB_LOGGING

// ReSharper disable CppInconsistentNaming
const char* TABLE_BOTS = "bots";
const char* COL_B_NAME = "username";

const char* CREATE_TABLE_BOTS = \
"CREATE TABLE bots ("
"   username      TEXT NOT NULL"
"   UNIQUE"
"   PRIMARY KEY"
");";

const char* TABLE_DETECTIONS = "detections";
const char* COL_D_NAME = "username";
const char* COL_D_IP = "ip";
const char* COL_D_VERSION = "version";
const char* COL_D_FLAGS = "flags";
const char* COL_D_SERVER = "server_name";
const char* COL_D_GAMEMODE = "gamemode";
const char* COL_D_VS_BOTS = "vs_bots";

const char* CREATE_TABLE_DETECTIONS = \
"CREATE TABLE detections ("
"   id       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
"   username TEXT REFERENCES bots (username)"
"                               NOT NULL,"
"   version      INTEGER NOT NULL,"
"   flags      INTEGER NOT NULL,"
"   ip      TEXT NOT NULL,"
"   server_name TEXT NOT NULL,"
"   gamemode TEXT NOT NULL,"
"   vs_bots BOOLEAN NOT NULL"
");";
// ReSharper restore CppInconsistentNaming

int execute_and_print(sqlite3* db, const char* statement, const sqlite3_callback x_callback, void* data, const char* message)
{
	char* message_error;
	const auto res = sqlite3_exec(db, statement, x_callback, data, &message_error);
	if (res != SQLITE_OK) {
		#ifdef DB_LOGGING
		std::cerr << message << ": Failed (" << message_error << ")" << std::endl;
		#endif
		sqlite3_free(message_error);
	}
	else
	{
		#ifdef DB_LOGGING
		std::cout << message << ": OK" << std::endl;
		#endif
	}
	return res;
}

static int user_version_callback(void* data, const int argc, char** argv, char** az_col_name)
{
	if (argc > 0) {
		static_cast<CDatabase*>(data)->user_version = atoi(argv[0]);
	}

	return 0;
}

#include <random>

int CDatabase::Open(const std::string& path)
{
	if (m_db != nullptr) {
		sqlite3_close(m_db);
	}
	auto res = sqlite3_open(path.c_str(), &m_db);

	if (res) {
		#ifdef DB_LOGGING
		std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
		#endif
		return res;
	}

	execute_and_print(m_db, "PRAGMA user_version;", user_version_callback, this, "Get User Version");
	res = execute_and_print(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, "Set PRAGMA foreign_keys = ON");

	if (user_version == 0)
	{
		sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

		res += execute_and_print(m_db, CREATE_TABLE_BOTS, nullptr, nullptr, "Create table bots");
		res += execute_and_print(m_db, CREATE_TABLE_DETECTIONS, nullptr, nullptr, "Create table detections");

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return res;
		}
		execute_and_print(m_db, "PRAGMA user_version = 2;", user_version_callback, this, "Set User Version to 2");

		res = sqlite3_exec(m_db, "END TRANSACTION;", nullptr, nullptr, nullptr);

		if (res != SQLITE_OK) {
			user_version = 2;
		}
	}

	if (user_version == 1)
	{
		sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

		res += execute_and_print(m_db, CREATE_TABLE_DETECTIONS, nullptr, nullptr, "Create table detections");

		if (res) {
			#ifdef DB_LOGGING
			std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
			return res;
		}
		execute_and_print(m_db, "PRAGMA user_version = 2;", user_version_callback, this, "Set User Version to 2");

		res = sqlite3_exec(m_db, "END TRANSACTION;", nullptr, nullptr, nullptr);

		if (res != SQLITE_OK) {
			user_version = 2;
		}
	}

	return res;
}

bool CDatabase::IsBot(const std::string& username)
{
	sqlite3_stmt* stmt = nullptr;
	const auto res = sqlite3_prepare_v2(
		m_db,
		sqlite3_mprintf("SELECT * FROM %s where %s == %Q;", TABLE_BOTS, COL_B_NAME, username.c_str()),
		-1,
		&stmt,
		nullptr);

	if (res) {
		#ifdef DB_LOGGING
		std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
		return false;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		return true;
	}

	return false;
}

bool CDatabase::DetectionTracked(
	const std::string& username,
	const std::string& ip,
	const std::string& server_name,
	const std::string& gamemode,
	const int version,
	const int flags,
	const bool vs_bots)
{
	sqlite3_stmt* stmt = nullptr;
	const auto res = sqlite3_prepare_v2(
		m_db,
		sqlite3_mprintf("SELECT * FROM %s where %s == %Q and %s == %Q  and %s == %Q  and %s == %Q  and %s == %d  and %s == %d  and %s == %d;",
			TABLE_DETECTIONS,
			COL_D_NAME,
			username.c_str(),
			COL_D_IP,
			ip.c_str(),
			COL_D_SERVER,
			server_name.c_str(),
			COL_D_GAMEMODE,
			gamemode.c_str(),
			COL_D_VERSION,
			version,
			COL_D_FLAGS,
			flags,
			COL_D_VS_BOTS,
			vs_bots
		),
		-1,
		&stmt,
		nullptr);

	if (res) {
		#ifdef DB_LOGGING
		std::cerr << "Error open DB " << sqlite3_errmsg(m_db) << std::endl;
			#endif
		return false;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		return true;
	}

	return false;
}

void CDatabase::AddBot(const std::string& username, const std::string& ip, const std::string& server_name, const std::string& gamemode, const int version, const int flags, const bool vs_bots)
{
	if (!IsBot(username)) {
		bool success = execute_and_print(
			m_db,
			sqlite3_mprintf("INSERT INTO %s(%s) VALUES(%Q);", TABLE_BOTS, COL_B_NAME, username.c_str()),
			nullptr,
			nullptr,
			"insert bot"
		) == SQLITE_OK;
	}
	if (!DetectionTracked(username, ip, server_name, gamemode, version, flags, vs_bots)) {
		bool success = execute_and_print(
			m_db,
			sqlite3_mprintf(
				"INSERT INTO %s(%s, %s, %s, %s, %s, %s, %s) VALUES(%Q, %Q, %Q, %Q, %d, %d, %d);",
				TABLE_DETECTIONS,
				COL_D_NAME,
				COL_D_IP,
				COL_D_SERVER,
				COL_D_GAMEMODE,
				COL_D_VERSION,
				COL_D_FLAGS,
				COL_D_VS_BOTS,
				username.c_str(),
				ip.c_str(),
				server_name.c_str(),
				gamemode.c_str(),
				version,
				flags,
				vs_bots
			),
			nullptr,
			nullptr,
			"insert ip"
		) == SQLITE_OK;
	}
}