#ifndef USERSTORE_H
#define USERSTORE_H

#include <string>

/*
 * UserStore — 用户认证模块
 *
 * 阶段一（默认）：内存 unordered_map，启动时从 users.conf 加载初始账号。
 * 阶段二（可选）：编译时加 -DUSE_SQLITE，后端切换为 SQLite，接口不变。
 *
 * 线程安全：Verify / Register 内部持锁，可被多个工作线程并发调用。
 */
class UserStore {
public:
    /*
     * 登录或注册。
     *   isLogin = true  → 登录：验证用户名+密码是否匹配。
     *   isLogin = false → 注册：用户名不存在则写入，已存在返回 false。
     * 返回 true 表示操作成功。
     */
    static bool Verify(const std::string& name,
                       const std::string& pwd,
                       bool isLogin);

    /*
     * 从文件加载初始用户表。
     * 文件格式：每行一个账号，"username password"，# 开头为注释。
     * 建议在 WebServer 构造函数中调用一次。
     */
    static void LoadFromFile(const char* path);

private:
    UserStore() = delete;
};

#endif // USERSTORE_H
