# This Python file uses the following encoding: utf-8

# if __name__ == "__main__":
#     pass


import subprocess
import os
import time
import random
import csv
import datetime
from pathlib import Path
import shutil
from typing import List
import getpass
import json
from dotenv import load_dotenv
from tenacity import (
    retry,
    stop_after_attempt,
    wait_random_exponential,
    retry_if_exception_type,
    retry_if_exception
)
from google.colab import drive, userdata
from google import genai
from google.api_core import exceptions

import requests
import json

# From link: https://langchain-ai.github.io/langgraph/concepts/low_level/?ref=blog.langchain.com#multiple-schemas

# Get the Gemini API Key
def get_gemini_api_key():
    if not os.environ.get("GEMINI_API_KEY"):
        try:
            os.environ["GEMINI_API_KEY"] = userdata.get('GEMINI_API_KEY')
        except KeyError:
            os.environ["GEMINI_API_KEY"] = getpass.getpass("Enter API key for Google Gemini: ")
########

# OPENROUTER_API_KEY
def get_openrouter_api_key():
    if not os.environ.get("OPENROUTER_API_KEY"):
        try:
          os.environ["OPENROUTER_API_KEY"] = userdata.get('OPENROUTER_API_KEY')
        except KeyError:
          os.environ["OPENROUTER_API_KEY"] = getpass.getpass("Enter API key for OpenRouter: ")

########

def get_google_drive_key():
    if not os.environ.get("GOOGLE_DRIVE_KEY"):
        try:
          os.environ["GOOGLE_DRIVE_KEY"] = userdata.get('GOOGLE_DRIVE_KEY')
        except KeyError:
          os.environ["GOOGLE_DRIVE_KEY"] = getpass.getpass("Enter API key for Google Drive: ")
########

def initGoogleDrive():
    try:
        # Source: https://medium.com/@wl8380/how-to-mount-google-drive-in-google-colab-c688ec8eccb7
        # Mount the Google Drive at /content/drive
        drive.mount('/content/gdrive/')

        # Verify by listing the files in the drive
        #!ls /content/gdrive/My\ Drive/Collab-Playground/

        load_dotenv()

        return (drive)

    except Exception as e:
        # Catch any errors (including the 403 Insufficient Permissions error)
        print(f"\n--- An Error Occurred ---")
        print(f"Error: {e}")

        if "403" in str(e):
            print("\n ACTION REQUIRED: This is likely an Insufficient Permissions error.")
            print("Check your API Key in the Google API documentation.")

    return (-1)

########

class VideoSegmentInfo:
    def __init__(self, path, id, start_time, end_time, duration, state):
        if start_time is None or end_time is None or duration is None:
            raise ValueError("All arguments must be provided.")
        self.path = path
        self.id = id
        self.start_time = start_time
        self.end_time = end_time
        self.duration = duration
        self.state = state

########

RETRYABLE_ERRORS = (
    genai.errors.ServerError, # Covers 500 (Internal), 502 (Ba Gateway), 503 (Unavailable), and 504 (Gateway Timeout)
    genai.errors.ClientError  # We will filter for 429 inside the function if needed
    )

LOG_FILE = "/content/gdrive/MyDrive/Colab-Playground/videos/segments/gemini_api_errors.csv"

def log_api_event(segment_path, status, error_msg="None"):
    with open(LOG_FILE, mode='a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            segment_path,
            status,
            error_msg
        ])

# 2. Define Retry Logic
def is_rate_limit(e):
    return isinstance(e, genai.errors.ClientError) and "429" in str(e)

########

retry_cond = retry_if_exception_type(genai.errors.ServerError) | retry_if_exception(is_rate_limit)

########

@retry(
    # Retry up to 5 times
    stop=stop_after_attempt(10),
    # Wait exponentially: 1s, 2s, 4s, 8s, up to 60s max
    wait=wait_random_exponential(multiplier=1, max=120),
    # Only retry on server errors or rate limits
    retry=retry_cond,
    # Optional: print a message when retrying
    before_sleep=lambda rs: log_api_event("gemini: unknown_segment", "RETRYING", str(rs.outcome.exception()))
)
def gemini_generate_with_retry(segment_path, client, model, contents, config=None):
    if not isinstance(client, genai.Client):
        raise ValueError("gemini_generate_with_retry(...) --- client must be an instance of genai.Client")

    try:
        response = client.models.generate_content(
            model=model,
            contents=contents,
            config=config
        )

        log_api_event(segment_path, "SUCCESS")

        return response

    except genai.errors.ClientError as e:
        # 1. Handle Rate Limits (429)
        # If it's a 429 (Rate Limit), tenacity will retry it due to retry_if_exception_type
        # If it's a 400 (Bad Request), we should NOT retry and instead raise it
        if "429" in str(e):
          print(f"DEBUG: Handle Rate Limits: {e}")
        if "400" in str(e):
          print(f"DEBUG: Check if offsets exceed video length: {e}")

        print(f"gemini_generate_with_retry(...) --- ❌ Permanent Client Error (400/403/404): {e}")

        log_api_event(segment_path, "PERMANENT_FAIL", str(e))

        raise e

    except Exception as e:
        log_api_event(segment_path, "PERMANENT_FAIL", str(e))
        raise e

########

def ffmpeg_check_duration_cmd(input_file):
  return [
      'ffprobe',
      '-v', 'error',
      '-show_entries', 'format=duration',
      '-of', 'default=noprint_wrappers=1:nokey=1',
      input_file
  ]

########

def get_video_duration(input_file):
    result = subprocess.run(ffmpeg_check_duration_cmd(input_file), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    if result.returncode != 0:
        print(f"Error running ffprobe (return code {result.returncode}):\n{result.stderr}")
        return None # Indicate failure

    duration_str = result.stdout.strip()

    if not duration_str:
        print(f"Error: ffprobe returned empty duration for '{input_file}'.\nStdout: '{result.stdout}'\nStderr: '{result.stderr}'")
        return None # Indicate failure

    try:
        return float(duration_str)
    except ValueError:
        print(f"Error: Could not convert ffprobe output '{duration_str}' to float for '{input_file}'.")
        return None # Indicate failure

########

def has_audio_quiet(file_path):
    """Checks if a video file has an audio track using ffprobe."""
    """ This function returns true if len(data.get('streams', [])) > 0. """

    command = [
        'ffprobe', '-v', 'quiet', '-show_streams',
        '-select_streams', 'a', '-print_format', 'json', file_path
    ]

    output = subprocess.check_output(command)
    data = json.loads(output)

    # If has_audio returns False, you should add a silent track:
    # ffmpeg -i input.mp4 -f lavfi -i anullsrc -c:v copy -c:a aac -shortest output.mp4

    return len(data.get('streams', [])) > 0

def has_audio(file_path):
    """ Checks if a video file has an audio track using ffprobe. """
    """ This function returns true or false. """

    command = ['ffprobe', '-v', 'error', '-select_streams', 'a', '-show_entries', 'stream=index', '-of', 'csv=p=0', file_path]

    has_audio = subprocess.run(command, capture_output=True, text=True).stdout.strip()

    # If has_audio returns False, you should add a silent track:
    # ffmpeg -i input.mp4 -f lavfi -i anullsrc -c:v copy -c:a aac -shortest output.mp4

    return has_audio

########

def get_status_message(status):
    match status:
        case 249:
            return "gemini-2.0-flash"
        case 250:
            return "gemini-2.5-flash"
        case 251:
            return "gemini-2.5-pro"
        case 252:
            return "gemini-2.5-ultra"
        case 253:
            return "gemini-2.5-ultra-plus"
        case 300:
            return "gemini-3-flash-preview"
        case 301:
            return "gemini-3.0-pro"
        case _:  # This is the wildcard (default case)
            return "Unknown Status"

########

def get_model_code(model):
    match model:
        case "gemini-2.0-flash":
            return 249
        case "gemini-2.5-flash":
            return 250
        case "gemini-2.5-pro":
            return 251
        case "gemini-2.5-ultra":
            return 252
        case "gemini-2.5-ultra-plus":
            return 253
        case "gemini-3-flash-preview":
            return 300
        case "gemini-3.0-pro":
            return 301
        case _:  # This is the wildcard (default case)
            return -1

########

def get_videos(segment_dir):

      if not os.path.exists(segment_dir): return

      files_in_dir = os.listdir(segment_dir)

    if files_in_dir is None or not isinstance(files_in_dir, list) or len(files_in_dir) == 0:
        print("get_videos(...) --- No video segments found. Exiting.")

        return None

    print(f"get_videos(...) --- Found {len(files_in_dir)} video segments in {segment_dir}.")

    start_time = 0
    end_time = start_time + CLIP_SEC
    clip_id = 0

    videoSegments = []

    for file_path in files_in_dir:
        if ".ipynb_checkpoints" in file_path:
            continue

        path = os.path.join(segment_dir, file_path)

    if os.path.isfile(path):
        duration = get_video_duration(path)

        if duration is None:
            print("Failed to get video duration. Exiting.")
            continue

        temp_path = path.replace(".mp4", "_audio_silence.mp4")

        result = subprocess.run(ffmpeg_ensure_audio_track_silence_cmd(path, temp_path), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

#        if result.returncode == 0:
#            print(f"get_videos(...) --- Error running ffmpeg (return code {result.returncode}):\n    {result.stderr}") # Indicate failure
#            os.remove(path)
#            os.rename(temp_path, path)
#        else:
#            os.remove(temp_path)

        base_path, ext = os.path.splitext(path)

        videoSeg = VideoSegmentInfo(path=path,
            id=base_path.split("_")[-5],
            start_time=start_time,
            end_time=start_time+duration,
            duration=duration,
            state="OK"
        )

        videoSegments.append(videoSeg)

        start_time += duration
        # end_time = start_time + CLIP_SEC

        print(f"\n get_videos(...) ---    Path: {videoSeg.path} ; id: {videoSeg.id}; start: {videoSeg.start_time} ; end: {videoSeg.end_time}; Duration: {videoSeg.duration}s")

    print(f"get_videos(...) --- Done: {len(videoSegments)}! ---")
    return videoSegments

########

def analyse_video(videoSegments: List, model: str, prompt: str):

    if videoSegments is None or not isinstance(videoSegments, list) or len(videoSegments) == 0:
        print("analyse_video(...) --- No video segments found. Exiting.")

        return False

    if model is None or not isinstance(model, str):
        print("analyse_video(...) --- No model provided. Exiting.")

        return False

    # Initialize the client
    client = genai.Client(api_key=os.environ.get("GEMINI_API_KEY"))

    try:
        model_info = client.models.get(model=model)
        print(f"analyse_video(...) --- ✅ Model exists! Display name: {model_info.display_name}")
    except Exception as e:
        print(f"analyse_video(...) --- ❌ Model does not exist or access is denied {e}")

        return False

    # Configuration
    total_files = len(videoSegments)  # Stop after all files have been processed
    processed_count = 0

    for segment in videoSegments:

        if processed_count >= total_files:
            print(f"analyse_video(...) --- Reached limit of {total_files}. Stopping.")

            break

        if not os.path.isfile(segment.path):
            print(f"analyse_video(...) --- File not found: {segment.path}")
            continue

        # 2. UPLOAD: Send to Gemini File API
        print(f"analyse_video(...) --- Uploading ({processed_count + 1}/{total_files}): {segment.path}...")
        video_file = client.files.upload(file=segment.path)

        # # 3. WAIT: Video must be ACTIVE before analysis
        while str(video_file.state) == "PROCESSING" or video_file.state == genai.types.FileState.PROCESSING :

            print(f"analyse_video(...) --- Waiting for {segment.path} to be ACTIVE (Current state: {video_file.state})...")
            segment.state = str(video_file.state)
            time.sleep(5)
            video_file = client.files.get(name=video_file.name)

        # Upload FAILED or STATE_UNSPECIFIED: Video upload failed of unspecified error
        if (str(video_file.state) == "FAILED" or video_file.state == genai.types.FileState.FAILED) or (str(video_file.state) == "STATE_UNSPECIFIED" or video_file.state == genai.types.FileState.STATE_UNSPECIFIED):
            segment.state = str(video_file.state)
            print(f"\n analyse_video(...) --- Upload failed: {video_file.state}.")
            continue

        # 4. ANALYZE: Send the prompt to Gemini 1.5 Flash (optimized for video speed)
        print(f"\n analyse_video(...) --- Analyzing with Gemini... {video_file.state}")

#        response = client.models.generate_content(
##            model="gemini-2.5-flash",
##            contents=[video_file, PROMPT]
##        )

##        response = gemini_generate_with_retry(
##            model="gemini-2.5-pro",
##            contents=[video_file, PROMPT],
##            config=genai.types.GenerateContentConfig(
##                video_metadata=genai.types.VideoMetadata(
##                    start_offset=f"{segment.start_time}s",
##                    end_offset=f"{segment.end_time}s"
##                )
##            )
##        )

#        # Build the Content Parts
#        video_part = genai.types.Part(
#            file_data=genai.types.FileData(
#                file_uri=video_file.uri,
#                mime_type=video_file.mime_type
#            ),
#            video_metadata=genai.types.VideoMetadata(
#                start_offset=f"{segment.start_time:.9f}s",
#                end_offset=f"{segment.end_time:.9f}s"
#            )
#        )

#        text_part = genai.types.Part(text=prompt)

#        config = genai.types.GenerateContentConfig(
#            thinking_config=genai.types.ThinkingConfig(
#                thinking_budget=0
#            ),
#            media_resolution=genai.types.MediaResolution.MEDIA_RESOLUTION_HIGH, # Improves detail for small objects/foul detection
#            temperature=0.1
#        )

        start_str=f"{int(segment.start_time)}s"
        end_str=f"{int(segment.end_time)}s"

        video_part = genai.types.Part.from_uri(
            file_uri=video_file.uri,
            mime_type="video/mp4",
        )

        video_part.video_metadata=genai.types.VideoMetadata(
            start_offset=start_str,
            end_offset=end_str
        )

        print(f"\n analyse_video(...) --- Prompt: {prompt}")

        text_part = genai.types.Part(text=prompt)

        config = genai.types.GenerateContentConfig(
            thinking_config=genai.types.ThinkingConfig(thinking_budget=0),
            temperature=0.1
        )

        print(f"\n analyse_video(...) --- Segment path {segment.path}")

        try:
            response = gemini_generate_with_retry(
                segment_path=segment.path,
                client=client,
                model=model,
                contents=[video_part, text_part],
                config=config
            )

#            response = client.models.generate_content(
#                model="gemini-2.5-flash",
#                contents=[video_part, text_part]
#            )

            # 5. RESULT: Output the findings
            print(f"analyse_video(...) --- RESULT FOR CLIP {segment.path}:\n{response.text}")

            client.files.delete(name=video_file.name)

            time.sleep(60) # Wait one minute for next request.

        except Exception as e:
            # Catch any errors (including the 403 Insufficient Permissions error)
            print(f"analyse_video(...) --- \n--- An Error Occurred ---")
            print(f"Error: {e}")

        # Cleanup: Delete the local segment file to save space
        # os.remove(file_path)

        processed_count += 1

    return True

########

def analyse_video_no_timestampts(videoSegments: List, model: str, prompt: str):

    if videoSegments is None or not isinstance(videoSegments, list) or len(videoSegments) == 0:
        print("analyse_video(...) --- No video segments found. Exiting.")

        return False

    if model is None or not isinstance(model, str):
        print("analyse_video(...) --- No model provided. Exiting.")

        return False

    # Initialize the client
    client = genai.Client(api_key=os.environ.get("GEMINI_API_KEY"))

    try:
        model_info = client.models.get(model=model)
        print(f"analyse_video(...) --- ✅ Model exists! Display name: {model_info.display_name}")

    except Exception as e:
        print(f"analyse_video(...) --- ❌ Model does not exist or access is denied {e}")

        return False

    # Configuration
    total_files = len(videoSegments)  # Stop after all files have been processed
    processed_count = 0

    for segment in videoSegments:

        if processed_count >= total_files:
            print(f"analyse_video(...) --- Reached limit of {total_files}. Stopping.")

            break

        if not os.path.isfile(segment.path):
            print(f"analyse_video(...) --- File not found: {segment.path}")
            continue

        # 2. UPLOAD: Send to Gemini File API
        print(f"analyse_video(...) --- Uploading ({processed_count + 1}/{total_files}): {segment.path}...")
        video_file = client.files.upload(file=segment.path)

        # # 3. WAIT: Video must be ACTIVE before analysis
        while str(video_file.state) == "PROCESSING" or video_file.state == genai.types.FileState.PROCESSING :
            print(f"analyse_video(...) --- Waiting for {segment.path} to be ACTIVE (Current state: {video_file.state})...")
            segment.state = str(video_file.state)
            time.sleep(5)
            video_file = client.files.get(name=video_file.name)

        # Upload FAILED or STATE_UNSPECIFIED: Video upload failed of unspecified error
        if (str(video_file.state) == "FAILED" or video_file.state == genai.types.FileState.FAILED) or (str(video_file.state) == "STATE_UNSPECIFIED" or video_file.state == genai.types.FileState.STATE_UNSPECIFIED):
            segment.state = str(video_file.state)
            print(f"\n analyse_video(...) --- Upload failed: {video_file.state}.")
            continue

        # 4. ANALYZE: Send the prompt to Gemini 1.5 Flash (optimized for video speed)
        print(f"\n analyse_video(...) --- Analyzing with Gemini... {video_file.state}")

        start_str=f"{int(0)}s"
        end_str=f"{int(segment.end_time - segment.start_time)}s"

        video_part = genai.types.Part.from_uri(
            file_uri=video_file.uri,
            mime_type="video/mp4",
        )

        video_part.video_metadata=genai.types.VideoMetadata(
            start_offset=start_str,
            end_offset=end_str
        )

        newprompt = f"{prompt} The start and end time of the video are Start: {segment.start_time} End: {segment.end_time}, respectively. Consider these times to provide the events timestamps."

        print(f"\n analyse_video(...) --- Prompt: {newprompt}")

        text_part = genai.types.Part(text=newprompt)

        config = genai.types.GenerateContentConfig(
            thinking_config=genai.types.ThinkingConfig(thinking_budget=0),
            temperature=0.1
        )

        print(f"\n analyse_video(...) --- Segment path {segment.path}")

        try:
            response = gemini_generate_with_retry(
                segment_path=segment.path,
                client=client,
                model=model,
                contents=[video_part, text_part],
                config=config
            )

            # 5. RESULT: Output the findings
            print(f"analyse_video(...) --- RESULT FOR CLIP {segment.path}:\n{response.text}")

            client.files.delete(name=video_file.name)

            time.sleep(60) # Wait one minute for next request.

        except Exception as e:
            # Catch any errors (including the 403 Insufficient Permissions error)
            print(f"analyse_video(...) --- \n--- An Error Occurred ---")
            print(f"Error: {e}")

            #Cleanup: Delete the local segment file to save space
            os.remove(file_path)

        processed_count += 1

  return True

########

def ffmpeg_sound_setup():
  '''
    -f lavfi -i anullsrc='channel_layout=stereo:sample_rate=44100': This creates a secondary input (Index 1). lavfi (Libavfilter) is an internal virtual device. anullsrc generates a "null" or silent audio stream with professional-standard settings (stereo, 44.1kHz).

    -map '1:a:0': Use silent audio. Specifically tells FFmpeg to use the audio from the second input (the silent stream) and ignore any original audio from the input video.
  '''
  return ['-f', 'lavfi', '-i', 'anullsrc=channel_layout=stereo:sample_rate=44100',
          '-map', '1:a:0',]

########

def fps_filter_complex_setup(fps):
  '''
    This is the core "engine" of the command:
      -filter_complex "[0:v]fps={fps}[v]":
        It takes the video from the first file (0:v).

        Applies the fps filter to change the frame rate to your variable (e.g., 10 or 5 FPS).

        Labels the resulting stream as [v] so it can be referenced later.

    -map '[v]': Specifically tells FFmpeg to use the processed, downsampled video for the output.
  '''

  return ['-filter_complex', f"[0:v]fps={fps}[v]",
          '-map', '[v]',]

########

def encoding_setup(crf):
  '''
    -c:v libx264: Encodes the video using the H.264 codec.
    -crf {crf}: Constant Rate Factor. Controls quality.
    -pix_fmt yuv420p: Ensures "Planar YUV 4:2:0" pixel format.
    -c:a aac: Encodes the silent audio into the AAC format.
    -shortest: Match duration. This is critical. Since anullsrc is an infinite stream, this flag tells FFmpeg to stop encoding as soon as the video ends.
    -preset slow: Tells the encoder to take more time to compress the file, resulting in better quality per megabyte.
  '''
  return ['-c:v', 'libx264',
          '-crf', str(crf),
          '-pix_fmt', 'yuv420p',
          '-c:a', 'aac',
          '-shortest',
          '-preset', 'slow',]

########

def slicer_setup(clip_sec):
  '''
    -f segment :Changes the output mode to segmentation.
    -segment_time {clip_sec}: The target length for each chunk (e.g., 60 seconds).
    -reset_timestamps 1: Each new segment file starts its internal clock at 00:00:00. This is essential for individual segments to be playable on their own.
  '''

  return ['-f', 'segment',
          '-segment_time', str(clip_sec),
          '-reset_timestamps', '1',]

########

import glob

def downsample_silence_and_segment_video(input_video, model, segment_dir, clip_sec=None, silence=None):
    """
    1. Validation & Setup.
    2. Prepares directory.
    3. Single-Pass FFmpeg Command.
    3.1 Adjusts FPS and injects silence and segments video into multiple files.
    4. Executes the FFMPEG pipeline.
    5. Gathers metadata and creates VideoSegmentInfo objects.
    5.1. Sort files to ensure chronological order.
    5.2. Extracting the numeric ID from the filename.
    5.3. Appends filename and metadata to the list of VideoSegmentInfo objects.
    6. Returns a list of VideoSegmentInfo objects.
    """
    # 1. Validation & Setup.
    if not os.path.exists(input_video):
        raise ValueError(f"downsample_silence_and_segment_video(...) --- Input file '{input_video}' does not exist.")

    # 2. Prepares directory.
    if not os.path.exists(segment_dir):
        os.makedirs(segment_dir)
        print(f"downsample_silence_and_segment_video(...) --- Created directory: {segment_dir}")

    code = get_model_code(model)
    config = {
        249: ("_gemini20_flash_10fps", 10, 23),
        250: ("_gemini25_flash_5fps", 5, 23),
        251: ("_gemini25_pro_10fps", 10, 20),
        252: ("_gemini25_ultra_5fps", 5, 18),
        253: ("_gemini25_ultra_plus_8fps", 8, 17),
        300: ("_gemini30_flash_10fps", 10, 21),
        301: ("_gemini30_pro_10fps", 10, 18),
    }

    downsampled_silence_str = "_downsampled_silence"

    if code not in config:
        raise ValueError(f"downsample_silence_and_segment_video(...) --- Unsupported model code: {code}")

    suffix, fps, crf = config[code]

    # Pattern for the segments
    output_pattern = os.path.join(segment_dir, f"segment{suffix}{downsampled_silence_str}_%03d.mp4")

    # 3. Single-Pass FFmpeg Command
    # 3.1 We combine FPS conversion, silence injection, and segments video into multiple files.
    cmd = [
        'ffmpeg',
        '-y',              # Global "yes" flag. It automatically overwrites any existing output files without asking for permission.
        '-i', input_video, # Your primary source (Index 0).
        ]

    if silence is not None:
        cmd.extend(ffmpeg_sound_setup())

    if fps is not None:
        cmd.extend(fps_filter_complex_setup(fps))

    cmd.extend(encoding_setup(crf))

    if clip_sec is not None:
        cmd.extend(slicer_setup(clip_sec))

    cmd.extend([output_pattern]) # Usually contains %03d.mp4. FFmpeg uses this to name files sequentially (e.g., seg_001.mp4, seg_002.mp4).

    #
    # 4. Executes the FFMPEG pipeline.
    print(f"downsample_silence_and_segment_video(...) --- Running FFMPEG pipeline for {model}...")
    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        raise ValueError(f"downsample_silence_and_segment_video(...) --- FFMPEG Error: {result.stderr}")

    # 5. Gathers metadata and creates VideoSegmentInfo objects.
    video_segments = []
    current_start_time = 0.0

    # 5.1. Sort files to ensure chronological order
    segment_files = sorted(glob.glob(os.path.join(segment_dir, f"segment{suffix}{downsampled_silence_str}_*.mp4")))

    for path in segment_files:
        duration = get_video_duration(path) # Assumed utility function exists

        if duration is None:
            print(f"downsample_silence_and_segment_video(...) --- Warning: Could not get duration for {path}")
            continue

        # 5.2. Extracting the numeric ID from the filename (e.g., '002' from 'segment_flash_10fps_002.mp4')
        try:
            segment_id = path.split('_')[-1].split('.')[0]
        except:
            segment_id = "000"

        video_seg = VideoSegmentInfo(
            path=path,
            id=segment_id,
            start_time=current_start_time,
            end_time=current_start_time + duration,
            duration=duration,
            state="OK"
        )

        # 5.3. Appends filename and metadata to list of VideoSegmentInfo objects.
        video_segments.append(video_seg)
        current_start_time += duration

        print(f"downsample_silence_and_segment_video(...) --- Segment Created: {path} | ID: {segment_id} | {duration}s")

    # 6. Returns a list of VideoSegmentInfo objects.
    print(f"downsample_silence_and_segment_video(...) --- Total Segments: {len(video_segments)}")
    return video_segments

########

# --- CONFIGURATION ---
INPUT_VIDEO = "/content/gdrive/MyDrive/Colab-Playground/videos/Brasil_x_Bolivia_Eliminatorias_da_Copa_2018_Jogo_Completo.mp4"

SEGMENTS_DIR = "/content/gdrive/MyDrive/Colab-Playground/videos/segments"
CLIP_SEC = 300  # 5 minutes
OVERLAP_SEC = 10
PROMPT = "Analyze this video for a 'Soccer event'. Provide the start and end timestamps in MM:SS format and a brief description."

PROMPT2 = "You are a very experienced football referee and a very attentive observer. Analyse this video and identify the football events and other events that occurred. Provide the start and end date/time stamps in MM:SS format and a brief description."

drive = initGoogleDrive()
get_gemini_api_key()

########

def test(videoSegments):
    try:
        # Configuration
        total_files = len(videoSegments) # Stop after all files have been processed
        print(f"test(...) --- Total files found: {total_files}")
        processed_count = 0

        for segment in videoSegments:
            print(f"test(...) --- Segment: {segment.path}")

            if not has_audio(segment.path):
                print(f"Skipping {segment.path}: No audio track detected (Known Gemini 500 trigger).")
                continue

            # for file_path in files_in_dir:
            if processed_count >= total_files:
                print(f"Reached limit of {total_files}. Stopping.")
                break

            print(f"test(...) --- Uploading ({processed_count + 1}/{total_files}): {segment.path}...")

#            exists = any(str(segment.path) == str(file_path) for segment in videoSegments)

            if os.path.isfile(segment.path):
                exists = True
            else:
                exists = False

            print(f"test(...) --- File path: {exists}...")

            if exists:
                print(f"test(...) --- File path: {segment.path}...")

    except Exception as e:
        # Catch any errors (including the 403 Insufficient Permissions error)
        print(f"\n--- An Error Occurred ---")
        print(f"Error: {e}")

########


########################################################################################################################

if __name__ == "__main__":

    print(f"Segmenting video!")
    videoSegments = downsample_silence_and_segment_video(input_video=INPUT_VIDEO, model="gemini-2.5-flash", segment_dir=SEGMENTS_DIR, clip_sec=CLIP_SEC)

    # videoSegments = get_videos("/content/gdrive/MyDrive/Colab-Playground/videos/segments/2.0-flash")

    print(f"Analizing video!")
    # analyse_video(videoSegments, "gemini-2.5-flash", PROMPT2)

    analyse_video_no_timestampts(videoSegments, "gemini-2.5-flash", PROMPT2)

    # test(SEGMENTS_DIR)

    # # 1. Initialize the client
    # client = genai.Client(api_key=os.environ.get("GEMINI_API_KEY"))

    # # 2. Use client.models.list() instead of genai.list_models()
    # print("Models supporting generateContent:")
    # for m in client.models.list():
    #     # In the new SDK, supported_actions is the field to check
    #     if "generateContent" in m.supported_actions:
    #         print(f"Model Name: {m.name}")


