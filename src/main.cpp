#include <iostream>
#include <csignal>
#include <string>
#include <optional>
#include <vector>

#include <minimal-audio-engine/coreengine.h>
#include <minimal-audio-engine/trackmanager.h>
#include <minimal-audio-engine/track.h>
#include <minimal-audio-engine/logger.h>

#ifndef VERSION_NUMBER
#define VERSION_NUMBER "1.0.0"
#endif

struct Command
{
  std::string argument;
  std::string argument_short;
  std::string description;
  std::function<void(const char *arg)> action;

  Command(const std::string& arg, const std::string& arg_short, const std::string& desc, std::function<void(const char *arg)> act = nullptr)
    : argument(arg), argument_short(arg_short), description(desc), action(act) {}
};

void help(const char *arg);

static std::string program_name = ""; 
static bool running = true;
static std::optional<std::string> input_file_path = std::nullopt;
static std::optional<unsigned int> audio_output_device_id = std::nullopt;

static const std::vector<Command> commands = {
  Command("--help", "-h", "Show help message", help),
  Command("--version", "-v", "Show version information", [](const char *){
    std::cout << "WavAudioPlayer Version " << VERSION_NUMBER << "\n";
    std::exit(0);
  }),
  Command("--input-file", "-i", "Specify input WAV file", [](const char *arg){
    input_file_path = std::string(arg);
  }),
  Command("--list-audio-devices", "-ld", "List available audio devices", [](const char *){
    auto& device_manager = MinimalAudioEngine::DeviceManager::instance();
    auto audio_devices = device_manager.get_audio_devices();
    std::cout << "Available Audio Devices:\n";
    for (const auto& device : audio_devices) {
      std::cout << "  ID: " << device.id << ", Name: " << device.name << ", (Input Channels: " << device.input_channels << ", Output Channels: " << device.output_channels << ")\n";
    }
    std::exit(0);
  }),
  Command("--set-audio-output", "-o", "Specify audio output device by ID", [](const char *arg){
    unsigned int device_id = std::stoi(arg);
    auto& device_manager = MinimalAudioEngine::DeviceManager::instance();
    auto audio_device = device_manager.get_audio_device(device_id);
    if (audio_device.id != device_id) {
      std::cerr << "Error: No audio device found with ID " << device_id << ".\n";
      std::exit(1);
    }
    std::cout << "Selected Audio Output Device: " << audio_device.to_string() << "\n";
    audio_output_device_id = device_id;
  })
};

void help(const char *arg)
{
  (void)arg;

  std::cout << "WavAudioPlayer - A simple WAV audio player using Minimal Audio Engine\n";
  std::cout << "Usage: " << program_name << " [options]\n\n";
  std::cout << "Options:\n";
  for (const auto &command : commands)
  {
    std::cout << "  " << command.argument << ", " << command.argument_short << "\t" << command.description << "\n";
  }
  std::cout << std::endl;
  std::exit(0);
}

void parse_command_line_arguments(int argc, char *argv[])
{
  if (argc < 2) {
    help(argv[0]);
    return;
  }

  program_name = argv[0];

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    for (const auto& command : commands) {
      if (strcmp(arg.c_str(), command.argument.c_str()) == 0 ||
          strcmp(arg.c_str(), command.argument_short.c_str()) == 0) {
        if ((i + 1) < argc)
        {
          command.action(argv[i + 1]);
        }
        else
        {
          command.action(nullptr);
        }
        break;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  parse_command_line_arguments(argc, argv);

  LOG_INFO("Initializing WavAudioPlayer...");

  MinimalAudioEngine::CoreEngine engine;

  // Resource managers
  MinimalAudioEngine::TrackManager &track_manager = MinimalAudioEngine::TrackManager::instance();
  MinimalAudioEngine::DeviceManager &device_manager = MinimalAudioEngine::DeviceManager::instance();
  MinimalAudioEngine::FileManager &file_manager = MinimalAudioEngine::FileManager::instance();

  if (input_file_path.has_value()) {
    if (!file_manager.is_wav_file(input_file_path.value())) {
      LOG_ERROR("Specified input file is not a valid WAV file: ", input_file_path.value());
      return -1;
    }
    LOG_INFO("WAV file to be played: ", input_file_path.value());
  } else {
    LOG_INFO("No input file specified. Exiting.");
    return 0;
  }

  engine.start_thread();

  std::signal(SIGINT, [](int) {
    LOG_INFO("SIGINT received, shutting down...");
    running = false;
  });

  // Add one track
  size_t track_id = track_manager.add_track();
  auto track = track_manager.get_track(track_id);
  if (!track) {
    LOG_ERROR("Failed to create track.");
    return -1;
  }

  // Set audio output device
  auto audio_output = (audio_output_device_id.has_value()) ? device_manager.get_audio_device(audio_output_device_id.value()) : device_manager.get_default_audio_output_device();
  if (audio_output.has_value()) {
    track->add_audio_device_output(audio_output.value());
    LOG_INFO("Set audio output device: ", audio_output->to_string());
  } else {
    LOG_ERROR("No default audio output device found.");
  }

  // Set WAV file as audio input
  auto wav_file = file_manager.read_wav_file(input_file_path.value());
  if (wav_file.has_value()) {
    track->add_audio_file_input(wav_file.value());
    LOG_INFO("Set WAV file as audio input: ", input_file_path.value());
  } else {
    LOG_ERROR("Failed to read WAV file: ", input_file_path.value());
    return -1;
  }
  
  // TODO - Set track properties (volume, pan, etc.)

  // Set callback for end of playback
  track->set_event_callback([](MinimalAudioEngine::eTrackEvent event) {
    if (event == MinimalAudioEngine::eTrackEvent::PlaybackFinished) {
      LOG_INFO("Track playback finished.");
      running = false;
    }
  });

  // Start playback
  track->play();

  // Main application loop
  while (engine.is_running()) {
    if (!running) {
      LOG_INFO("Shutting down engine...");
      track->stop();
      engine.stop_thread();
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}