#include <iostream>
#include <gst/gst.h>
#include <string>
#include <unistd.h>
#include <termios.h>

class VideoPlayer {
public:
    VideoPlayer(const std::string &file_path) {
        gst_init(nullptr, nullptr);
        pipeline = gst_element_factory_make("playbin", "player");

        if (!pipeline) {
            std::cerr << "Failed to create GStreamer pipeline!" << std::endl;
            exit(1);
        }

        // Set file path
        std::string uri = "file://" + file_path;
        g_object_set(pipeline, "uri", uri.c_str(), NULL);
    }

    void play() {
        std::cout << "Playing..." << std::endl;
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }

    void pause() {
        std::cout << "Paused." << std::endl;
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
    }

    void stop() {
        std::cout << "Stopped." << std::endl;
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }

    void seek_forward() {
        seek_by_seconds(10);  // Move forward by 10 seconds
    }

    void seek_backward() {
        seek_by_seconds(-10); // Move backward by 10 seconds
    }

    ~VideoPlayer() {
        gst_object_unref(pipeline);
    }

private:
    GstElement *pipeline;

    void seek_by_seconds(gint64 seconds) {
        GstFormat fmt = GST_FORMAT_TIME;
        gint64 current_pos;

        if (gst_element_query_position(pipeline, fmt, &current_pos)) {
            gint64 new_position = current_pos + (seconds * GST_SECOND);
            std::cout << "Seeking to " << new_position / GST_SECOND << " seconds" << std::endl;
            gst_element_seek_simple(pipeline, fmt, GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), new_position);
        } else {
            std::cerr << "Failed to get current position!" << std::endl;
        }
    }
};

// Function to set terminal to read keys without Enter
void set_non_blocking_input(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

int main() {
    std::string video_file;
    std::cout << "Enter the video file path: ";
    std::getline(std::cin, video_file);

    VideoPlayer player(video_file);

    std::cout << "Controls: \n"
              << "[SPACE] - Play/Pause\n"
              << "[→] - Forward 10s\n"
              << "[←] - Backward 10s\n"
              << "[q] - Quit\n";

    player.play();  // Auto start playback

    set_non_blocking_input(true); // Enable non-blocking key input

    while (true) {
        char key;
        if (read(STDIN_FILENO, &key, 1) > 0) {
            if (key == ' ') {
                static bool is_paused = false;
                is_paused = !is_paused;
                is_paused ? player.pause() : player.play();
            } else if (key == 'q') {
                player.stop();
                break;
            } else if (key == '\033') { // Arrow key sequence
                char next;
                read(STDIN_FILENO, &next, 1);
                if (next == '[') {
                    char arrow;
                    read(STDIN_FILENO, &arrow, 1);
                    if (arrow == 'C') { // Right Arrow
                        player.seek_forward();
                    } else if (arrow == 'D') { // Left Arrow
                        player.seek_backward();
                    }
                }
            }
        }
        usleep(10000); // Small delay to reduce CPU usage
    }

    set_non_blocking_input(false); // Restore terminal settings
    return 0;
}

