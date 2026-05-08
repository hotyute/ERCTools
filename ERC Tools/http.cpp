// =================================================================================
// FILE: http.cpp
// =================================================================================


#include "http.h"
#include "util.h"

struct InternetHandle
{
    HINTERNET h = nullptr;

    InternetHandle() = default;
    explicit InternetHandle(HINTERNET handle) : h(handle) {}
    ~InternetHandle()
    {
        if (h)
            WinHttpCloseHandle(h);
    }

    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
};


bool QueryHttpStatus(HINTERNET request, DWORD& statusOut, std::wstring& errorOut)
{
    statusOut = 0;
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status,
        &statusSize,
        WINHTTP_NO_HEADER_INDEX))
    {
        errorOut = L"Could not read HTTP status: " + WinErrorText(GetLastError());
        return false;
    }

    statusOut = status;
    return true;
}

bool EnsureHttpSuccess(HINTERNET request, std::wstring& errorOut)
{
    DWORD status = 0;
    if (!QueryHttpStatus(request, status, errorOut))
        return false;

    if (status >= 200 && status < 300)
        return true;

    wchar_t reason[512]{};
    DWORD reasonSize = sizeof(reason);
    if (!WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_TEXT,
        WINHTTP_HEADER_NAME_BY_INDEX,
        reason,
        &reasonSize,
        WINHTTP_NO_HEADER_INDEX))
    {
        reason[0] = L'\0';
    }

    errorOut = L"HTTP " + std::to_wstring(status);
    if (reason[0] != L'\0') {
        errorOut += L" ";
        errorOut += reason;
    }

    if (status >= 300 && status < 400) {
        wchar_t location[2048]{};
        DWORD locationSize = sizeof(location);
        if (WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_LOCATION,
            WINHTTP_HEADER_NAME_BY_INDEX,
            location,
            &locationSize,
            WINHTTP_NO_HEADER_INDEX) && location[0] != L'\0')
        {
            errorOut += L" (redirect to ";
            errorOut += location;
            errorOut += L")";
        }
    }

    return false;
}

void ConfigureSecureProtocols(HINTERNET session)
{
    DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
    secureProtocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
    WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
}

void ConfigureRedirects(HINTERNET request)
{
#ifdef WINHTTP_OPTION_REDIRECT_POLICY
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));
#endif
}

bool HttpGetText(const std::wstring& inputUrl, std::string& bodyOut, std::wstring& errorOut)
{
    bodyOut.clear();
    errorOut.clear();

    std::wstring url = NormalizeUrl(inputUrl);
    if (url.empty()) {
        errorOut = L"Empty URL.";
        return false;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);

    wchar_t host[2048]{};
    wchar_t path[4096]{};
    wchar_t extra[4096]{};

    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = _countof(extra);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        errorOut = L"Could not parse URL: " + WinErrorText(GetLastError());
        return false;
    }

    if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) {
        errorOut = L"Only http:// and https:// URLs are supported.";
        return false;
    }

    std::wstring hostName(host, parts.dwHostNameLength);
    std::wstring objectName(path, parts.dwUrlPathLength);
    std::wstring extraInfo(extra, parts.dwExtraInfoLength);
    std::wstring fullPath = objectName + extraInfo;
    if (fullPath.empty())
        fullPath = L"/";

    InternetHandle session(WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) ERCTools/1.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));

    if (!session.h) {
        errorOut = L"WinHttpOpen failed: " + WinErrorText(GetLastError());
        return false;
    }

    WinHttpSetTimeouts(session.h, 15000, 15000, 15000, 15000);

    ConfigureSecureProtocols(session.h);

    INTERNET_PORT port = parts.nPort;
    if (port == 0)
        port = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    InternetHandle connect(WinHttpConnect(session.h, hostName.c_str(), port, 0));
    if (!connect.h) {
        errorOut = L"WinHttpConnect failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    InternetHandle request(WinHttpOpenRequest(
        connect.h,
        L"GET",
        fullPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags));

    if (!request.h) {
        errorOut = L"WinHttpOpenRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    ConfigureRedirects(request.h);

    std::wstring headers =
        L"Accept: text/html,application/xhtml+xml,application/geo+json,application/json,text/json,text/plain,*/*;q=0.8\r\n"
        L"Accept-Encoding: identity\r\n"
        L"Accept-Language: en-GB,en;q=0.9\r\n"
        L"Cache-Control: no-cache\r\n"
        L"Referer: https://www.trafficengland.com/traffic-alerts\r\n"
        L"X-Requested-With: XMLHttpRequest\r\n";

    WinHttpAddRequestHeaders(request.h, headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        errorOut = L"WinHttpSendRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!WinHttpReceiveResponse(request.h, nullptr)) {
        errorOut = L"WinHttpReceiveResponse failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!EnsureHttpSuccess(request.h, errorOut))
        return false;

    std::vector<unsigned char> bytes;
    bytes.reserve(4096);

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available)) {
            errorOut = L"WinHttpQueryDataAvailable failed: " + WinErrorText(GetLastError());
            return false;
        }

        if (available == 0)
            break;

        size_t oldSize = bytes.size();
        bytes.resize(oldSize + available);

        DWORD read = 0;
        if (!WinHttpReadData(request.h, bytes.data() + oldSize, available, &read)) {
            errorOut = L"WinHttpReadData failed: " + WinErrorText(GetLastError());
            return false;
        }

        bytes.resize(oldSize + read);
    }

    if (bytes.size() >= 3 &&
        bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
    {
        bytes.erase(bytes.begin(), bytes.begin() + 3);
    }

    bodyOut.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

bool HttpGetBinary(const std::wstring& inputUrl, std::vector<BYTE>& bodyOut, std::wstring& errorOut)
{
    bodyOut.clear();
    errorOut.clear();

    std::wstring url = NormalizeUrl(inputUrl);
    if (url.empty()) {
        errorOut = L"Empty URL.";
        return false;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);

    wchar_t host[2048]{};
    wchar_t path[4096]{};
    wchar_t extra[4096]{};

    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = _countof(extra);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        errorOut = L"Could not parse URL: " + WinErrorText(GetLastError());
        return false;
    }

    if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) {
        errorOut = L"Only http:// and https:// URLs are supported.";
        return false;
    }

    std::wstring hostName(host, parts.dwHostNameLength);
    std::wstring objectName(path, parts.dwUrlPathLength);
    std::wstring extraInfo(extra, parts.dwExtraInfoLength);
    std::wstring fullPath = objectName + extraInfo;
    if (fullPath.empty())
        fullPath = L"/";

    InternetHandle session(WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) ERCTools/1.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));

    if (!session.h) {
        errorOut = L"WinHttpOpen failed: " + WinErrorText(GetLastError());
        return false;
    }

    WinHttpSetTimeouts(session.h, 15000, 15000, 15000, 15000);
    ConfigureSecureProtocols(session.h);

    INTERNET_PORT port = parts.nPort;
    if (port == 0)
        port = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    InternetHandle connect(WinHttpConnect(session.h, hostName.c_str(), port, 0));
    if (!connect.h) {
        errorOut = L"WinHttpConnect failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    InternetHandle request(WinHttpOpenRequest(
        connect.h,
        L"GET",
        fullPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags));

    if (!request.h) {
        errorOut = L"WinHttpOpenRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    ConfigureRedirects(request.h);

    std::wstring headers =
        L"Accept: image/png,image/*;q=0.9,*/*;q=0.8\r\n"
        L"Accept-Encoding: identity\r\n";

    WinHttpAddRequestHeaders(request.h, headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        errorOut = L"WinHttpSendRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!WinHttpReceiveResponse(request.h, nullptr)) {
        errorOut = L"WinHttpReceiveResponse failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!EnsureHttpSuccess(request.h, errorOut))
        return false;

    std::vector<BYTE> bytes;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available)) {
            errorOut = L"WinHttpQueryDataAvailable failed: " + WinErrorText(GetLastError());
            return false;
        }

        if (available == 0)
            break;

        size_t oldSize = bytes.size();
        bytes.resize(oldSize + available);

        DWORD read = 0;
        if (!WinHttpReadData(request.h, bytes.data() + oldSize, available, &read)) {
            errorOut = L"WinHttpReadData failed: " + WinErrorText(GetLastError());
            return false;
        }

        bytes.resize(oldSize + read);
    }

    bodyOut = std::move(bytes);
    return true;
}


bool IsTrafficEnglandAlertsPageUrl(const std::wstring& inputUrl)
{
    std::wstring url = ToLower(NormalizeUrl(inputUrl));

    const size_t fragment = url.find(L'#');
    if (fragment != std::wstring::npos)
        url.resize(fragment);

    const size_t query = url.find(L'?');
    if (query != std::wstring::npos)
        url.resize(query);

    while (!url.empty() && url.back() == L'/')
        url.pop_back();

    return url == L"https://www.trafficengland.com/traffic-alerts" ||
        url == L"https://trafficengland.com/traffic-alerts" ||
        url == L"http://www.trafficengland.com/traffic-alerts" ||
        url == L"http://trafficengland.com/traffic-alerts";
}

std::wstring BuildTrafficEnglandAlertsApiUrl(size_t start, size_t step, bool unplannedOnly, const std::wstring& order)
{
    const ULONGLONG cacheBuster = GetTickCount64();
    std::wstring url = L"https://www.trafficengland.com/api/events/getAlerts";
    url += L"?start=" + std::to_wstring(start);
    url += L"&step=" + std::to_wstring(step);
    std::wstring safeOrder = order.empty() ? L"Road" : order;
    url += L"&order=" + safeOrder;
    url += L"&is_current=1";
    url += unplannedOnly
        ? L"&events=CONGESTION,FULL_CLOSURES,INCIDENT,WEATHER,ABNORMAL_LOADS"
        : L"&events=CONGESTION,FULL_CLOSURES,ROADWORKS,INCIDENT,WEATHER,MAJOR_ORGANISED_EVENTS,ABNORMAL_LOADS";
    url += L"&unconfirmed=false";
    url += L"&completed=false";
    url += L"&includeUnconfirmedRoadworks=true";
    url += L"&_=" + std::to_wstring(cacheBuster + start);
    return url;
}

bool HttpPostJsonText(const std::wstring& inputUrl, const std::string& jsonBody, std::string& bodyOut, std::wstring& errorOut)
{
    bodyOut.clear();
    errorOut.clear();

    std::wstring url = NormalizeUrl(inputUrl);
    if (url.empty()) {
        errorOut = L"Empty URL.";
        return false;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);

    wchar_t host[2048]{};
    wchar_t path[4096]{};
    wchar_t extra[4096]{};

    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = _countof(extra);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        errorOut = L"Could not parse URL: " + WinErrorText(GetLastError());
        return false;
    }

    if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) {
        errorOut = L"Only http:// and https:// URLs are supported.";
        return false;
    }

    std::wstring hostName(host, parts.dwHostNameLength);
    std::wstring objectName(path, parts.dwUrlPathLength);
    std::wstring extraInfo(extra, parts.dwExtraInfoLength);
    std::wstring fullPath = objectName + extraInfo;
    if (fullPath.empty())
        fullPath = L"/";

    InternetHandle session(WinHttpOpen(
        L"ERCTools/1.2",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));

    if (!session.h) {
        errorOut = L"WinHttpOpen failed: " + WinErrorText(GetLastError());
        return false;
    }

    WinHttpSetTimeouts(session.h, 10000, 10000, 10000, 10000);
    ConfigureSecureProtocols(session.h);

    INTERNET_PORT port = parts.nPort;
    if (port == 0)
        port = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    InternetHandle connect(WinHttpConnect(session.h, hostName.c_str(), port, 0));
    if (!connect.h) {
        errorOut = L"WinHttpConnect failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    InternetHandle request(WinHttpOpenRequest(
        connect.h,
        L"POST",
        fullPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags));

    if (!request.h) {
        errorOut = L"WinHttpOpenRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    ConfigureRedirects(request.h);

    std::wstring headers =
        L"Content-Type: application/json; charset=utf-8\r\n"
        L"Accept: application/json,text/plain,*/*;q=0.8\r\n"
        L"Accept-Encoding: identity\r\n";

    const DWORD bodySize = static_cast<DWORD>(jsonBody.size());
    LPVOID optionalData = bodySize ? static_cast<LPVOID>(const_cast<char*>(jsonBody.data())) : WINHTTP_NO_REQUEST_DATA;
    if (!WinHttpSendRequest(
        request.h,
        headers.c_str(),
        -1L,
        optionalData,
        bodySize,
        bodySize,
        0))
    {
        errorOut = L"WinHttpSendRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!WinHttpReceiveResponse(request.h, nullptr)) {
        errorOut = L"WinHttpReceiveResponse failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!EnsureHttpSuccess(request.h, errorOut))
        return false;

    std::vector<unsigned char> bytes;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available)) {
            errorOut = L"WinHttpQueryDataAvailable failed: " + WinErrorText(GetLastError());
            return false;
        }

        if (available == 0)
            break;

        size_t oldSize = bytes.size();
        bytes.resize(oldSize + available);

        DWORD read = 0;
        if (!WinHttpReadData(request.h, bytes.data() + oldSize, available, &read)) {
            errorOut = L"WinHttpReadData failed: " + WinErrorText(GetLastError());
            return false;
        }

        bytes.resize(oldSize + read);
    }

    bodyOut.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}
