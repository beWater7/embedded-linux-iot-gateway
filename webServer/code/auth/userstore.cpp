#include "userstore.h"
#include "../log/log.h"

#include <fstream>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include <string>

/* ------------------------------------------------------------------ */
/* 阶段一：内存实现                                                      */
/* ------------------------------------------------------------------ */
#ifndef USE_SQLITE

namespace {
    std::unordered_map<std::string, std::string> s_users;
    std::mutex s_mtx;
}

void UserStore::LoadFromFile(const char* path)
{
    if (!path) return;

    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_WARN("UserStore: cannot open %s, starting with empty user table.", path);
        return;
    }

    std::string line;
    int count = 0;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string name, pwd;
        if (!(ss >> name >> pwd)) continue;
        std::lock_guard<std::mutex> lock(s_mtx);
        s_users[name] = pwd;
        count++;
    }
    LOG_INFO("UserStore: loaded %d user(s) from %s", count, path);
}

bool UserStore::Verify(const std::string& name,
                       const std::string& pwd,
                       bool isLogin)
{
    if (name.empty() || pwd.empty()) return false;

    std::lock_guard<std::mutex> lock(s_mtx);

    if (isLogin) {
        auto it = s_users.find(name);
        if (it == s_users.end()) {
            LOG_DEBUG("UserStore: login failed, user '%s' not found.", name.c_str());
            return false;
        }
        if (it->second != pwd) {
            LOG_DEBUG("UserStore: login failed, wrong password for '%s'.", name.c_str());
            return false;
        }
        LOG_DEBUG("UserStore: user '%s' logged in.", name.c_str());
        return true;
    } else {
        /* 注册：用户名已存在则拒绝 */
        if (s_users.count(name)) {
            LOG_DEBUG("UserStore: register failed, '%s' already exists.", name.c_str());
            return false;
        }
        s_users[name] = pwd;
        LOG_DEBUG("UserStore: user '%s' registered (in-memory).", name.c_str());
        return true;
    }
}

/* ------------------------------------------------------------------ */
/* 阶段二：SQLite 实现（编译时 -DUSE_SQLITE 启用）                        */
/* ------------------------------------------------------------------ */
#else  /* USE_SQLITE */

#include <sqlite3.h>

/*
 * SQLite 数据库路径。可改为绝对路径，方便嵌入式环境部署。
 * 数据库在 LoadFromFile 时打开/创建，表结构自动初始化。
 */
#define SQLITE_DB_PATH  "resources/users.db"

namespace {
    sqlite3 *s_db = nullptr;
    std::mutex s_mtx;

    /* 确保 users 表存在 */
    void ensureTable() {
        const char *sql =
            "CREATE TABLE IF NOT EXISTS users("
            "  username TEXT PRIMARY KEY,"
            "  password TEXT NOT NULL"
            ");";
        char *err = nullptr;
        if (sqlite3_exec(s_db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            LOG_ERROR("UserStore(sqlite): create table error: %s", err);
            sqlite3_free(err);
        }
    }
}

void UserStore::LoadFromFile(const char* path)
{
    std::lock_guard<std::mutex> lock(s_mtx);

    if (sqlite3_open(SQLITE_DB_PATH, &s_db) != SQLITE_OK) {
        LOG_ERROR("UserStore(sqlite): open %s failed: %s",
                  SQLITE_DB_PATH, sqlite3_errmsg(s_db));
        s_db = nullptr;
        return;
    }
    ensureTable();

    /* 将 users.conf 中的初始账号导入数据库（幂等：INSERT OR IGNORE） */
    if (!path) {
        LOG_INFO("UserStore(sqlite): opened %s, no conf to import.", SQLITE_DB_PATH);
        return;
    }
    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_WARN("UserStore(sqlite): cannot open %s, skip import.", path);
        return;
    }
    std::string line;
    int count = 0;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string name, pwd;
        if (!(ss >> name >> pwd)) continue;

        const char *insertSql =
            "INSERT OR IGNORE INTO users(username,password) VALUES(?,?);";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(s_db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, pwd.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            count++;
        }
    }
    LOG_INFO("UserStore(sqlite): imported %d user(s) from %s", count, path);
}

bool UserStore::Verify(const std::string& name,
                       const std::string& pwd,
                       bool isLogin)
{
    if (name.empty() || pwd.empty()) return false;

    std::lock_guard<std::mutex> lock(s_mtx);

    if (!s_db) {
        LOG_ERROR("UserStore(sqlite): database not open.");
        return false;
    }

    if (isLogin) {
        const char *sql =
            "SELECT password FROM users WHERE username=? LIMIT 1;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(s_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return false;
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

        bool ok = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *stored = reinterpret_cast<const char*>(
                                     sqlite3_column_text(stmt, 0));
            ok = (stored && pwd == stored);
        }
        sqlite3_finalize(stmt);
        LOG_DEBUG("UserStore(sqlite): login '%s' -> %s", name.c_str(), ok?"ok":"fail");
        return ok;
    } else {
        const char *sql =
            "INSERT OR IGNORE INTO users(username,password) VALUES(?,?);";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(s_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return false;
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, pwd.c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        int changed = sqlite3_changes(s_db);
        sqlite3_finalize(stmt);
        LOG_DEBUG("UserStore(sqlite): register '%s' -> %s",
                  name.c_str(), changed ? "ok" : "exists");
        return (changed > 0);
    }
}

#endif /* USE_SQLITE */
