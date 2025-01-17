import json
from collections import defaultdict
import numpy as np
import csv
import codecs
import random
import os

hex_to_key = {
    "20": "space",  # Example mapping; add more as needed
    "7f": "backspace"  # Handle special keys explicitly if needed
}

string_to_code ={
    " ": "space"
}

def find_weak_words(weakest_info, corpus_file="word_corpus.txt", default_size=20):

    if os.path.isfile("wordlist.tmp"):
        os.remove("wordlist.tmp");

    weakest_letter = weakest_info['weakest_letter']
    weakest_pairs = weakest_info['weakest_pairs']

    # Read the word corpus from the file and ensure uniqueness
    with open(corpus_file, "r") as file:
        word_corpus = " ".join(list(set(file.read().splitlines()))).split()

    # Filter words containing the weakest letter
    filtered_words = [word for word in word_corpus if weakest_letter in word]

    # Sort words by priority of weakest pairs
    prioritized_words = []
    for pair in weakest_pairs:
        prioritized_words.extend([word for word in filtered_words if pair in word])

    # Include remaining words that were not part of weakest pairs
    remaining_words = [word for word in filtered_words if word not in prioritized_words]

    # Combine prioritized words and remaining words, ensuring uniqueness
    unique_words = list(dict.fromkeys(prioritized_words + remaining_words))[:50]

    # Randomly sample from the unique words to generate a string
    sampled_words = random.sample(unique_words, min(default_size, len(unique_words)))
    generated_string = " ".join(sampled_words)

    # Write the unique words to a file
    with open("wordlist.tmp", "w") as file:
        file.write(generated_string)


class PerformanceAnalyzer:
    def __init__(self, history_file="typing_history.json"):
        self.history_file = history_file
        self.data = self._load_history()

    def _load_history(self):
        try:
            with open(self.history_file, "r") as file:
                data = json.load(file)
                # Ensure all entries have a "weakness" field
                for letter in data.get("letters", {}):
                    data["letters"][letter].setdefault("weakness", 0)
                for pair in data.get("pairs", {}):
                    data["pairs"][pair].setdefault("weakness", 0)
                return data
        except FileNotFoundError:
            return {"letters": defaultdict(lambda: {"cpm": 0, "error_rate": 0, "weakness": 0, "sessions": []}),
                    "pairs": defaultdict(lambda: {"speed": 0, "count": 0, "error_rate": 0, "weakness": 0, "sessions": []})}

    def _save_history(self):
        with open(self.history_file, "w") as file:
            json.dump(self.data, file, indent=4)

    def analyze_session(self, session_data, target_string):
        letter_metrics = defaultdict(lambda: {"cpm": [], "error_rate": 0, "errors": 0, "typed_count": 0})
        pair_metrics = defaultdict(lambda: {"speed": [], "count": 0, "errors": 0, "error_rate": 0})

        typed_string = ""
        target_index = 0

        for i, entry in enumerate(session_data):
            key = entry["key"]
            print(f"key {key}")
            time = entry.get("time", 0)

            # Handle backspace
            if key == "backspace":
                if typed_string:
                    typed_string = typed_string[:-1]
                if target_index > 0:
                    target_index -= 1
                continue

            typed_string += key
            if key in string_to_code:
                letter_metrics[string_to_code[key]]["typed_count"] += 1
            else:
                letter_metrics[key]["typed_count"] += 1

            # Check for letter errors
            if target_index < len(target_string):
                expected_char = target_string[target_index]
                if expected_char in string_to_code:
                    expected_char = string_to_code[expected_char]
                print(f"expected char {expected_char} {key}")
                if key != expected_char:
                    letter_metrics[key]["errors"] += 1
                    letter_metrics[expected_char]["errors"] += 1
                    target_index += 1
                    if i > 0:
                        prev_key = session_data[i - 1]["key"]
                        pair = prev_key + key
                        pair_metrics[pair]["errors"] += 1
                else:
                    target_index += 1
            else:
                letter_metrics[key]["errors"] += 1
                if i > 0:
                    prev_key = session_data[i - 1]["key"]
                    if(prev_key != "backspace"):
                        pair = prev_key + key
                        pair_metrics[pair]["errors"] += 1

            # Calculate CPM for letters
            if time > 0:
                cpm = 60 / time
                letter_metrics[key]["cpm"].append(cpm)

            # Track letter pairs
            if i > 0:
                prev_key = session_data[i - 1]["key"]
                if(prev_key != "backspace"):
                    pair = prev_key + key
                    pair_metrics[pair]["speed"].append(time)
                    pair_metrics[pair]["count"] += 1

        # Update error rates and historical data for letters
        for letter, metrics in letter_metrics.items():
            total_errors = metrics["errors"]
            total_typed = metrics["typed_count"]
            if letter in string_to_code:
                letter = string_to_code[letter]
            print(f"letter {letter} errors{total_errors} typed {total_typed}")
            error_rate = total_errors / total_typed if total_typed > 0 else 0

            if error_rate > 1:
                error_rate = 1

            historical = self.data["letters"].get(letter, {"cpm": 0, "error_rate": 0, "weakness": 0, "sessions": []})
            avg_cpm = np.mean(metrics["cpm"]) if metrics["cpm"] else 0
            historical_cpm = historical.get("cpm", 0)
            historical_sessions = historical.get("sessions", [])

            # Weighted average for CPM
            updated_cpm = (historical_cpm * len(historical_sessions) + avg_cpm) / (len(historical_sessions) + 1)


            self.data["letters"][letter] = {
                "cpm": updated_cpm,
                "error_rate": error_rate,
                "weakness": updated_cpm * (1 - error_rate) if error_rate != 1 and error_rate > 0 else updated_cpm,
                "sessions": historical_sessions + [avg_cpm]
            }

        # Update error rates and historical data for pairs
        for pair, metrics in pair_metrics.items():
            total_errors = metrics["errors"]
            total_count = metrics["count"]
            error_rate = total_errors / total_count if total_count > 0 else 0

            if error_rate > 1:
                error_rate = 1

            historical = self.data["pairs"].get(pair, {"speed": 0, "count": 0, "error_rate": 0, "weakness": 0, "sessions": []})
            avg_speed = np.mean(metrics["speed"]) if metrics["speed"] else 0
            historical_speed = historical.get("speed", 0)
            historical_count = historical.get("count", 0)
            historical_sessions = historical.get("sessions", [])

            # Weighted average for transition speed
            updated_speed = (historical_speed * historical_count + avg_speed * total_count) / (historical_count + total_count if historical_count + total_count > 0 else 1)

            self.data["pairs"][pair] = {
                "speed": updated_speed,
                "count": historical_count + total_count,
                "error_rate": error_rate,
                "weakness": updated_speed * (1 - error_rate) if error_rate != 1 and error_rate > 0 else updated_speed,
                "sessions": historical_sessions + [avg_speed]
            }

        self._save_history()

    def identify_weak_points(self):
        # Find weakest letter based on weakness metric
        weakest_letter = min(self.data["letters"].items(),
                             key=lambda item: item[1].get("weakness", float("inf")))[0]

        # Find weakest pairs associated with weakest letter
        weakest_pairs = [pair for pair in self.data["pairs"] if weakest_letter in pair]
        weakest_pairs.sort(key=lambda pair: self.data["pairs"][pair].get("weakness", 0), reverse=True)

        return {
            "weakest_letter": weakest_letter,
            "weakest_pairs": weakest_pairs[:3]
        }

    def generate_metrics_summary(self):
        return {
            "letters": {letter: {"cpm": data["cpm"], "error_rate": data["error_rate"], "weakness": data["weakness"]}
                         for letter, data in self.data["letters"].items()},
            "pairs": {pair: {"speed": data["speed"], "count": data["count"], "error_rate": data.get("error_rate", 0), "weakness": data.get("weakness", 0)}
                       for pair, data in self.data["pairs"].items()}
        }


def csv_to_session_data(file_path):

    session_data = []

    with open(file_path, "r") as file:
        reader = csv.DictReader(file)
        for row in reader:
            key_hex = row["Key"].strip()
            key = hex_to_key.get(key_hex, codecs.decode(key_hex, "hex").decode("utf-8"))
            time_seconds = int(row["Time (microseconds)"].strip()) / 1_000_000
            session_data.append({"key": key, "time": round(time_seconds, 3)})

    return session_data

# Example Usage
if __name__ == "__main__":
    analyzer = PerformanceAnalyzer()
    session_data = csv_to_session_data("timing_data.csv")
    target_string = "hello world test"

    analyzer.analyze_session(session_data, target_string)
    weakest_points = analyzer.identify_weak_points()
    metrics_summary = analyzer.generate_metrics_summary()

    print("Weakest Points:", weakest_points)
    print("Metrics Summary:", json.dumps(metrics_summary, indent=4))

    find_weak_words(weakest_points)
