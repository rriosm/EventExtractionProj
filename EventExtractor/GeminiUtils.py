# This Python file uses the following encoding: utf-8

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

import google.auth
import googleapiclient.discovery

print(f"Google Auth version: {google.auth.__version__}")

try:
    # Attempt to initialize a client (even without a key) to check for import errors
    client = genai.Client(api_key="TEST")
    print("✅ google-genai is ready!")
except Exception as e:
    print(f"❌ Error with google-genai: {e}")

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
             print("\nACTION REQUIRED: This is likely an Insufficient Permissions error.")
             print("Check your API Key in the Google API documentation.")

    return (-1)

########

# Define file paths

class BaseDirPath:
    def __init__(self, b_DirPath):
        if isinstance(b_DirPath, str):
            self.BASE_DIR = b_DirPath
        else:
            self.BASE_DIR = None

        def isEmpty(self):
            if self.BASE_DIR == None:
                return True
            else:
                return False

########

class VideoFilePath:
    def __init__(self, v_FilePath):
        if isinstance(v_FilePath, str):
            self.VIDEO_PATH = v_FilePath
        else:
            self.VIDEO_PATH = None

    def isEmpty(self):
        if self.VIDEO_PATH == None:
            return True
        else:
            return False

########

class ShotsDirPath:
    def __init__(self, f_DirPath):
        if isinstance(f_DirPath, str):
            self.SHOTS_DIR = f_DirPath
        else:
            self.SHOTS_DIR = None

    def isEmpty(self):
        if self.SHOTS_DIR == None:
            return True
        else:
            return False

########

def delete_dir_and_contents(path_to_dir):
    # Deletes only the directory and its content.

    dir_to_delete = Path(path_to_dir)

    if dir_to_delete.exists() and dir_to_delete.is_dir():
        shutil.rmtree(dir_to_delete)

    if not dir_to_delete.exists():
        print(f"delete_dir_and_contents(...) --- The directory '{path_to_dir}' does not exist.")
        return True
    elif not dir_to_delete.is_dir():
        print(f"delete_dir_and_contents(...) --- '{path_to_dir}' is not a directory.")
        return False

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

def ffmpeg_seg_cmd(input_file, output_file, start_time, end_time):
  # Using re-encoding (-c:v libx264) for better timestamp accuracy in AI models

  # '-copyts',
  video_filters = f"setpts=PTS+{start_time}/TB"

  cmd = [
        'ffmpeg',
        '-y',
        '-i', input_file,
        '-vf', video_filters,
        '-t', str(300),
        '-c:v', 'libx264',
        '-c:a', 'copy',
        '-crf', '23',
        '-preset', 'slow',
        '-avoid_negative_ts', 'make_zero',
        output_file
  ]

  # cmd = [
  #       'ffmpeg',
  #       '-y',
  #       '-i', input_file,
  #       '-ss', str(start_time),
  #   ]

  # if end_time:
  #   cmd.extend(['-t', str(end_time - start_time)])

  # cmd.extend([
  #       '-c:v', 'libx264',
  #       '-c:a', 'copy',
  #       '-crf', '23',
  #       '-preset',
  #       'veryfast',
  #       '-avoid_negative_ts', 'make_zero',
  #       str(output_file)
  #   ])

  return cmd

########

def ffmpeg_seg_cmd_frame_perfection_cmd(input_file, output_file, duration):
  exprstr = f"expr:gte(t,n_forced*{int(duration)})"

  cmd = [
        'ffmpeg',
        '-y',
        '-i', input_file,
        '-force_key_frames', exprstr,
        '-f', 'segment',
        '-segment_time', str(duration),
        '-reset_timestamps', '1',
        output_file
  ]

  return cmd

########

def ffmpeg_seg_cmd_stream_copy_cmd(input_file, output_file, duration):
  """
    output_pattern should be something like 'segment_%03d.mp4'
  """

  cmd = [
        'ffmpeg',
        '-y',
        '-i', input_file,
        '-c', 'copy',
        '-map', '0',
        '-f', 'segment',
        '-segment_time', str(duration),
        '-reset_timestamps', '1',
        output_file
  ]

  return cmd

########

def ffmpeg_ensure_audio_track_silence_cmd(input_file, output_file):

  # Inject silence: -f lavfi -i anullsrc creates the silence
  # -c:v copy keeps original video quality/speed
  # '-copyts',

  return [
      'ffmpeg',
      '-y',
      '-i', input_file,
      '-f', 'lavfi',
      '-i', 'anullsrc=channel_layout=stereo:sample_rate=44100',
      '-map', '0:v:0',
      '-map', '1:a:0',
      '-c:v', 'libx264',
      '-c:a', 'aac',
      '-shortest',
      output_file
  ]

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

def ffmpeg_downsample_video(input_video, model, start_time_sec=0):
  """
    Downsamples video and offsets internal timestamps to match original video time.

    Args:
        input_video (str): Path to input file.
        model (str): Gemini model string.
        start_time_sec (int): The offset in seconds from the original master video.
  """

  if input_video is None or not isinstance(input_video, str):
    raise ValueError(f"ffmpeg_downsample_video(...) --- No input video provided.")

  if not os.path.exists(input_video):
    raise ValueError(f"ffmpeg_downsample_video(...) --- Input file '{input_video}' does not exist.")

  if model is None or not isinstance(model, str):
    raise ValueError(f"ffmpeg_downsample_video(...) --- No model provided.")

  # temp_output_video = input_video.replace(".mp4", "_downsampled.mp4")
  # temp_output_video = input_video.removesuffix(".mp4")
  base_path, ext = os.path.splitext(input_video)
  code = get_model_code(model)

  # Configuration Mapping
  # Format: (suffix, target_fps, target_width, crf)
  config = {
      249: ("_gemini_flash_10fps_downsampled", 10, 1280, 23),
      250: ("_gemini_flash_5fps_downsampled", 5, 1280, 23),
      251: ("_gemini_pro_10fps_downsampled", 10, 1920, 20),
      252: ("_gemini_ultra_5fps_downsampled", 5, 1920, 18),
      253: ("_gemini_ultra_plus_8fps_downsampled", 8, 1920, 17),
      300: ("_gemini3_flash_10fps_downsampled", 10, 1920, 21), # Check these paramenters!!!
      301: ("_gemini3_pro_10fps_downsampled", 10, 1920, 18),
  }

  if code not in config:
    raise ValueError(f"ffmpeg_downsample_video(...) --- Unsupported model code: {code} for model: {model}")

  suffix, fps, width, crf = config[code]
  temp_output = f"{base_path}{suffix}_at_{start_time_sec}s{ext}"

  # Construct the Filter String
  # 1. Set FPS
  # 2. Scale (using -2 to ensure even dimensions)
  # 3. setpts: Shifts timestamps by start_time_sec. /TB converts seconds to TimeBase units.
  # video_filters = f"fps={fps},scale={width}:-2,setpts=PTS+{start_time_sec}/TB"
  video_filters = f"fps={fps},setpts=PTS+{start_time_sec}/TB"

  cmd = [
        'ffmpeg',
        '-y',
        '-i', input_video,
        '-vf', video_filters,
        '-c:v', 'libx264',
#        '-t', str(300),
        '-crf', str(crf),
       '-pix_fmt', 'yuv420p',
        '-c:a', 'copy',
        '-preset', 'slow',
        temp_output
  ]

  # # .with_stem removes the extension and lets you add a new part
  # temp_output_video = Path(input_video).with_stem(Path(input_video).stem + "_downsampled")
  # # If you need it back as a string:
  # temp_output_video = str(temp_output_video.with_suffix(".mp4"))

  print(f"downsample_video(...) --- Running FFMPEG for {model} (Offset: {start_time_sec}s) --- cmd: {cmd} ---")

  result = subprocess.run(cmd, capture_output=True, text=True)

  if result.returncode != 0:
    raise ValueError(f"ffmpeg_downsample_video(...) --- FFMPEG Error (Code {result.returncode}):\n    {result.stderr}") # Indicate failure

  os.remove(input_video)
  # os.rename(temp_output, input_video)

  print(f"ffmpeg_downsample_video(...) --- Input file path: {input_video}")

  return temp_output

########

def segment_video(input_video, segment_dir, model):

    duration = float(get_video_duration(input_video))

    if duration is None:
        print("Failed to get video duration. Exiting.")
        return # Exit if duration could not be determined

    start_time = 0
    end_time = start_time + CLIP_SEC
    clip_id = 0

    if not os.path.exists(segment_dir):
        os.makedirs(segment_dir)
        print(f"segment_video(...) --- Created directory: {segment_dir}")
    else:
        print(f"segment_video(...) --- Directory already exists: {segment_dir}")

    videoSegments = []

    while start_time < duration:
        # 1. SEGMENT: Extract the clip using FFmpeg
        output_path = os.path.join(segment_dir, f"clip_{clip_id:03d}.mp4")

        print(f"\n segment_video(...) --- Processing Segment {clip_id} (Start: {start_time}s; End: {end_time}) ---")
        print(f"\n segment_video--- File path: {output_path}")

        # # Create video segment.
        # result = subprocess.run(ffmpeg_seg_cmd(input_video, output_path, start_time, end_time), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        result = subprocess.run(ffmpeg_seg_cmd_stream_copy(input_video, output_path, CLIP_SEC), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        print(f"\n 22222 segment_video(...) --- FFMPEG cmd: {result}")

        if result.returncode != 0:

            delete_dir_and_contents(segment_dir)

            raise ValueError(f"segment_video(...) --- Error running ffmpeg_seg_cmd (return code {result.returncode}):\n    {result.stderr}") # Indicate failure

        new_output_path = ffmpeg_downsample_video(output_path, model, start_time)

        print(f"segment_video(...) --- Ouput file path: {new_output_path}")

        if new_output_path is None or not isinstance(new_output_path, str) or not os.path.exists(new_output_path):
            # delete_dir_and_contents(segment_dir)
            raise ValueError(f"segment_video(...) --- Error running ffmpeg_downsample_video (return code {result.returncode}):\n    {result.stderr}") # Indicate failure

        # Check segment for audio
#        if not has_audio(new_output_path):
        print(f"Adding silent audio track to {new_output_path}...")
        temp_path = new_output_path.replace(".mp4", "_audio_silence.mp4")

        print(f"segment_video(...) --- Temp file path: {temp_path}")

        result = subprocess.run(ffmpeg_ensure_audio_track_silence_cmd(new_output_path, temp_path), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        if result.returncode == 0:
            print(f"segment_video(...) --- No error running ffmpeg (return code {result.returncode}):\n    {result.stderr}") # Indicate failure

            os.remove(new_output_path)
            os.rename(temp_path, new_output_path)

        else:
            print(f"segment_video(...) --- Error running ffmpeg (return code {result.returncode}):\n    {result.stderr}") # Indicate failure
            os.remove(temp_path)

        # Update segment duration
        print(f"segment_video(...) --- Checking segment duration...")

        result = subprocess.run(ffmpeg_check_duration_cmd(new_output_path), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        if result.returncode != 0:
            raise ValueError(f"segment_video(...) --- Error running ffprobe (return code {result.returncode}):\n    {result.stderr}") # Indicate failure

        else:

            duration_str = result.stdout.strip()

        if not duration_str:
            # Indicate failure
            raise ValueError(f"segment_video(...) --- Error: ffprobe returned empty duration for '{new_output_path}'.\nStdout: '{result.stdout}'\nStderr: '{result.stderr}'")
        else:
            print(f"segment_video(...) --- Segment duration: {duration_str}\n")

            try:
                videoSeg = VideoSegmentInfo(path=new_output_path,
                    id=clip_id,
                    start_time=start_time,
                    end_time=end_time,
                    duration=float(duration_str),
                    state="OK"
                )

                videoSegments.append(videoSeg)

#                print(f"\n segment_video(...) ---    Path: {videoSeg.path} ; id: {videoSeg.id}; start: {videoSeg.start_time} ; end: {videoSeg.end_time}; Duration: {videoSeg.duration}s; state: {videoSeg.state}")

            except ValueError:
                raise ValueError(f"segment_video(...) --- Error: Could not convert ffprobe output '{duration_str}' to float for '{output_path}'.") # Indicate failure

        # Prepare for next segment
        start_time += (CLIP_SEC - OVERLAP_SEC)
        end_time = start_time + CLIP_SEC
        clip_id += 1

    print(f"\n segment_video(...) --- Done: {len(videoSegments)}! ---")

    return videoSegments

########

def segment_video_and_downsample(input_video, segment_dir):
    if not os.path.exists(segment_dir):
        os.makedirs(segment_dir)
        print(f"segment_video_and_downsample(...) --- Created directory: {segment_dir}")
    else:
        print(f"segment_video_and_downsample(...) --- Directory already exists: {segment_dir}")

    output_path = os.path.join(segment_dir, f"segment_%03d.mp4")

    result = subprocess.run(ffmpeg_seg_cmd_stream_copy(input_video, output_path, CLIP_SEC), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    if result.returncode != 0:
        raise ValueError(f"segment_video_and_downsample(...) --- Error running ffmpeg_seg_cmd (return code {result.returncode}):\n    {result.stderr}") # Indicate failure

    if not os.path.exists(segment_dir): return

    files_in_dir = os.listdir(segment_dir)

    if files_in_dir is None or not isinstance(files_in_dir, list) or len(files_in_dir) == 0:
        print("segment_video_and_downsample(...) --- No video segments found. Exiting.")

        return None

    print(f"segment_video_and_downsample(...) --- Found {len(files_in_dir)} video segments in {segment_dir}.")

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
                print("segment_video_and_downsample(...) --- Failed to get video duration. Exiting.")
                continue

            temp_path = path.replace(".mp4", "_audio_silence.mp4")

            result = subprocess.run(ffmpeg_ensure_audio_track_silence_cmd(path, temp_path), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

            if result.returncode == 0:
                print(f"segment_video_and_downsample(...) --- No error running ffmpeg (return code {result.returncode}):\n    {result.stderr}") # Indicate failure

                os.remove(path)
                # os.rename(temp_path, path)

            else:
                os.remove(temp_path)

            base_path, ext = os.path.splitext(temp_path)

            videoSeg = VideoSegmentInfo(path=temp_path,
                                      id=base_path.split("_")[-3],
                                      start_time=start_time,
                                      end_time=start_time+duration,
                                      duration=duration,
                                      state="OK")

            videoSegments.append(videoSeg)

            start_time += duration

            print(f"\n segment_video_and_downsample(...) ---    Path: {videoSeg.path} ; id: {videoSeg.id}; start: {videoSeg.start_time} ; end: {videoSeg.end_time}; Duration: {videoSeg.duration}s")

    print(f"segment_video_and_downsample(...) --- Done: {len(videoSegments)}! ---")
    return videoSegments

########

def gemini_generate_with_retry_old(client, model, contents, config=None, max_retries=5):
    if not isinstance(client, genai.Client):
        raise ValueError("gemini_generate_with_retry_old(...) --- client must be an instance of genai.Client")

    retries = 0

    # while True:
    while retries < max_retries:
        try:
            response = client.models.generate_content(
                model=model,
                contents=contents,
                config=config
            )

            return response

        except Exception as e:
            error_msg = str(e).upper()

            # 1. Handle Rate Limits (429)
            if "429" in error_msg or "RESOURCE_EXHAUSTED" in error_msg:
                wait_time = 60
                print(f"gemini_generate_with_retry_old(...) --- Quota reached. Sleeping for {wait_time}s...")
                time.sleep(wait_time)
                # Note: We don't increment 'retries' for quota hits usually

            # 2. Handle Internal Errors (500) and Overloaded (503)
            elif any(code in error_msg for code in ["500", "503", "INTERNAL", "UNAVAILABLE"]):
                retries += 1
                # Exponential backoff: 2, 4, 8, 16... plus a bit of 'jitter'
                wait_time = (2 ** retries) + random.uniform(0, 1)
                print(f"gemini_generate_with_retry_old(...) --- Server error ({error_msg[:15]}...). Retrying {retries}/{max_retries} in {wait_time:.1f}s...")
                time.sleep(wait_time)

            else:
                # 3. Stop for critical errors (Invalid API Key, Safety Filters, etc.)
                print(f"gemini_generate_with_retry_old(...) --- Non-retriable error: {e}")
                raise e

    raise Exception(f"gemini_generate_with_retry_old(...) --- Failed after {max_retries} attempts due to persistent server errors.")

########

RETRYABLE_ERRORS = (
    genai.errors.ServerError, # Covers 500 (Internal), 502 (Ba Gateway), 503 (Unavailable), and 504 (Gateway Timeout)
    genai.errors.ClientError  # We will filter for 429 inside the function if needed
)

LOG_FILE = "/content/gdrive/MyDrive/Colab-Playground/videos/segments/gemini_api_errors.csv"

########

def log_api_event(segment_path, status, error_msg="None"):
    with open(LOG_FILE, mode='a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            segment_path,
            status,
            error_msg
        ])

########

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

@retry(
    # Retry up to 5 times
    stop=stop_after_attempt(10),
    # Wait exponentially: 1s, 2s, 4s, 8s, up to 60s max
    wait=wait_random_exponential(multiplier=1, max=120),
    # Only retry on server errors or rate limits
    retry=retry_cond,
    # Optional: print a message when retrying
    before_sleep=lambda rs: log_api_event("openrouter: unknown_segment", "RETRYING", str(rs.outcome.exception()))
)
def openrouter_generate_with_retry(segment_path, client, model, contents, config=None):
    if not isinstance(client, genai.Client):
        raise ValueError("openrouter_generate_with_retry(...) --- client must be an instance of genai.Client")

    try:
        response = requests.post(
            url="https://openrouter.ai/api/v1/chat/completions",
            headers={
                "Authorization": "Bearer <OPENROUTER_API_KEY>",
                "HTTP-Referer": "<YOUR_SITE_URL>", # Optional. Site URL for rankings on openrouter.ai.
                "X-Title": "<YOUR_SITE_NAME>", # Optional. Site title for rankings on openrouter.ai.
            },
            data=json.dumps({
                "model": "openai/gpt-4o", # Optional
                "messages": [
                    {
                        "role": "user",
                        "content": "What is the meaning of life?"
                    }
                  ]
            })
        )

        log_api_event(segment_path, "SUCCESS")

        return response

    except genai.errors.ClientError as e:
        # 1. Handle Rate Limits (429)
        # If it's a 429 (Rate Limit), tenacity will retry it due to retry_if_exception_type
        # If it's a 400 (Bad Request), we should NOT retry and instead raise it
        if "429" in str(e):
            print(f"openrouter_generate_with_retry(...) --- ❌ Permanent Client Error (400/403/404): {e}")

        log_api_event(segment_path, "PERMANENT_FAIL", str(e))

        raise e

    except Exception as e:
        log_api_event(segment_path, "PERMANENT_FAIL", str(e))
        raise e

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

        # temp_path = path.replace(".mp4", "_audio_silence.mp4")

        # result = subprocess.run(ffmpeg_ensure_audio_track_silence_cmd(path, temp_path), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        # if result.returncode == 0:
        #   print(f"get_videos(...) --- Error running ffmpeg (return code {result.returncode}):\n    {result.stderr}") # Indicate failure

        #   os.remove(path)
        #   os.rename(temp_path, path)

        # else:
        #   os.remove(temp_path)

        base_path, ext = os.path.splitext(path)

        videoSeg = VideoSegmentInfo(path=path,
                                  id=base_path.split("_")[-1],
                                  start_time=start_time,
                                  end_time=start_time+duration,
                                  duration=duration,
                                  state="OK")

        videoSegments.append(videoSeg)

        start_time += duration
        # end_time = start_time + CLIP_SEC

        print(f"\n get_videos(...) ---    Path: {videoSeg.path} ; id: {videoSeg.id}; start: {videoSeg.start_time} ; end: {videoSeg.end_time}; Duration: {videoSeg.duration}s")

    print(f"get_videos(...) --- Done: {len(videoSegments)}! ---")
    return videoSegments

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

      # exists = any(str(segment.path) == str(file_path) for segment in videoSegments)

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
