/*
 * @Author       : mark
 * @Date         : 2020-06-26
 * @copyleft Apache 2.0
 */
#include <algorithm>
#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture",
};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0}, {"/login.html", 1},
};

void HttpRequest::Init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const
{
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive"
               && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff)
{
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0) return false;

    while (buff.ReadableBytes() && state_ != FINISH) {
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(),
                                     CRLF, CRLF + 2);
        string line(buff.Peek(), lineEnd);
        switch (state_) {
        case REQUEST_LINE:
            if (!ParseRequestLine_(line)) return false;
            ParsePath_();
            break;
        case HEADERS:
            ParseHeader_(line);
            if (buff.ReadableBytes() <= 2) state_ = FINISH;
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if (lineEnd == buff.BeginWrite()) break;
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_()
{
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        for (auto& item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

/*
 * 请求行解析：替换 std::regex 为 sscanf，减小 ARM 二进制体积。
 * 格式：METHOD SP Request-URI SP HTTP/VERSION CRLF
 */
bool HttpRequest::ParseRequestLine_(const string& line)
{
    char method[16]  = {};
    char path[512]   = {};
    char version[16] = {};

    if (sscanf(line.c_str(), "%15s %511s HTTP/%15s", method, path, version) == 3) {
        method_  = method;
        path_    = path;
        version_ = version;
        state_   = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error: %s", line.c_str());
    return false;
}

/*
 * 头部行解析：替换 std::regex 为手动查找冒号。
 * 格式：Field-Name ":" OWS Field-Value
 */
void HttpRequest::ParseHeader_(const string& line)
{
    size_t colon = line.find(':');
    if (colon == string::npos) {
        /* 空行或无效行 → 进入 BODY 状态 */
        state_ = BODY;
        return;
    }
    string key   = line.substr(0, colon);
    /* 跳过冒号后的可选空格 */
    size_t valStart = colon + 1;
    while (valStart < line.size() && line[valStart] == ' ') valStart++;
    string value = line.substr(valStart);
    header_[key] = value;
}

void HttpRequest::ParseBody_(const string& line)
{
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%zu", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch)
{
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch - '0';
}

void HttpRequest::ParseFromUrlencoded_()
{
    if (body_.empty()) return;

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            if (i + 2 < n) {
                num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
            }
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

void HttpRequest::ParsePost_()
{
    if (method_ == "POST" &&
        header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            bool isLogin = (tag == 1);
            if (UserVerify(post_["username"], post_["password"], isLogin)) {
                path_ = "/welcome.html";
            } else {
                path_ = "/error.html";
            }
        }
    }
}

/*
 * 委托给 UserStore::Verify，不再直接操作数据库。
 */
bool HttpRequest::UserVerify(const string& name, const string& pwd, bool isLogin)
{
    if (name.empty() || pwd.empty()) return false;
    LOG_INFO("Verify name:%s", name.c_str());
    return UserStore::Verify(name, pwd, isLogin);
}

string HttpRequest::path() const  { return path_; }
string& HttpRequest::path()       { return path_; }
string HttpRequest::method() const { return method_; }
string HttpRequest::version() const { return version_; }

string HttpRequest::GetPost(const string& key) const
{
    if (post_.count(key) == 1)
        return post_.find(key)->second;
    return "";
}

string HttpRequest::GetPost(const char* key) const
{
    if (key && post_.count(key) == 1)
        return post_.find(key)->second;
    return "";
}
