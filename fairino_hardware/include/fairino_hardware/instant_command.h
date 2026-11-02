#ifndef _INSTANT_COMMAND_H
#define _INSTANT_COMMAND_H

// Controller "instant commands": short frames on TCP port 8080 that act at once, even while a blocking
// MoveJ/MoveL holds the SDK. The Fairino Python SDK (fairino/Robot.py) sends PauseMotion and ResumeMotion
// this way rather than over XML-RPC (send_message, with the XML-RPC calls commented out), so the driver
// does the same.
//
// Frame: /f/bIII<counter>III<command id>III<payload length>III<payload>III/b/f, counter 0 as in the SDK.
// Reply: the same six fields; field 4 is "1" when the controller accepted the command.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace fairino_instant_command
{
    constexpr int kPort = 8080;
    constexpr const char *kFieldSeparator = "III";
    constexpr const char *kFrameEnd = "/b/f";
    constexpr size_t kReplyFields = 6;
    constexpr size_t kReplyStatusField = 4;
    constexpr const char *kAccepted = "1";
    // The SDK reads one 1024-byte reply; more than that is not a reply to a short command.
    constexpr size_t kMaxReplyBytes = 1024;

    struct Command
    {
        int id;
        const char *payload;
    };
    // Command ids and payloads as the SDK sends them.
    constexpr Command kPause{103, "PAUSE"};
    constexpr Command kResume{104, "RESUME"};

    inline std::string frame(const Command &command)
    {
        const std::string payload = command.payload;
        const std::string sep = kFieldSeparator;
        return "/f/b" + sep + "0" + sep + std::to_string(command.id) + sep + std::to_string(payload.size()) + sep +
               payload + sep + kFrameEnd;
    }

    inline std::vector<std::string> splitFields(const std::string &text)
    {
        std::vector<std::string> fields;
        const std::string sep = kFieldSeparator;
        size_t start = 0;
        for (size_t pos; (pos = text.find(sep, start)) != std::string::npos; start = pos + sep.size())
            fields.push_back(text.substr(start, pos - start));
        fields.push_back(text.substr(start));
        return fields;
    }

    /** The controller's reply accepts the command (same rule as the SDK's send_message). */
    inline bool accepted(const std::string &reply)
    {
        const auto fields = splitFields(reply);
        return fields.size() == kReplyFields && fields[kReplyStatusField] == kAccepted;
    }

    /**
     * Sends one frame on a fresh connection and reads the reply.
     *
     * @param timeout bounds connect, send and each receive
     * @param reply   what the controller answered (possibly empty)
     * @param error   empty unless the transport failed
     * @return the frame was sent and a reply read
     */
    inline bool send(const std::string &ip, int port, const std::string &frameText, timeval timeout,
                     std::string &reply, std::string &error)
    {
        reply.clear();
        error.clear();
        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            error = "socket() failed";
            return false;
        }
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));  // also bounds connect() on Linux
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
            close(fd);
            error = "connect to " + ip + ":" + std::to_string(port) + " failed";
            return false;
        }
        size_t sent = 0;
        while (sent < frameText.size()) {
            const ssize_t n = ::send(fd, frameText.data() + sent, frameText.size() - sent, MSG_NOSIGNAL);
            if (n <= 0) {
                close(fd);
                error = "send to " + ip + ":" + std::to_string(port) + " failed";
                return false;
            }
            sent += static_cast<size_t>(n);
        }
        // Read until the frame end, the peer closes, a timeout, or the size cap: TCP may split the reply.
        char buf[256];
        while (reply.size() < kMaxReplyBytes && reply.find(kFrameEnd) == std::string::npos) {
            const ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n <= 0)
                break;
            reply.append(buf, static_cast<size_t>(n));
        }
        close(fd);
        if (reply.empty()) {
            error = "no reply from " + ip + ":" + std::to_string(port);
            return false;
        }
        return true;
    }
}  // namespace fairino_instant_command

#endif
