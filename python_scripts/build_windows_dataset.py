import pandas as pd
from pathlib import Path

# Config
DATA_DIR = Path(".")
WINDOW_SIZE = 50   # 1 s @ 50 Hz
STEP_SIZE = 25     # 50% overlap

SIGNAL_COLS = [
    "pitch1",
    "roll1",
    "pitch2",
    "roll2",
    "delta_pitch",
    "fsr1_scaled",
    "fsr2_scaled",
    "fsr_total",
    "fsr_balance",
]


def load_all_sessions():
    csv_paths = sorted(DATA_DIR.glob("good_*.csv")) + sorted(DATA_DIR.glob("bad_*.csv"))
    if not csv_paths:
        raise FileNotFoundError("No good_*.csv or bad_*.csv files found in this folder.")

    all_sessions = []
    for i, path in enumerate(csv_paths):
        print(f"Loading {path} ...")
        df = pd.read_csv(path)
        df = df.dropna(how="all")

        df["session_id"] = i
        df["source_file"] = path.name
        df["label"] = df["label"].astype(int)

        all_sessions.append(df)

    return all_sessions


def window_session(df, window_size=50, step_size=25):
    n = len(df)
    label = int(df["label"].iloc[0])
    session_id = df["session_id"].iloc[0]
    source_file = df["source_file"].iloc[0]

    rows = []
    for start in range(0, n - window_size + 1, step_size):
        stop = start + window_size
        w = df.iloc[start:stop]

        feat = {}
        for col in SIGNAL_COLS:
            arr = w[col].values.astype(float)
            feat[f"{col}_mean"] = arr.mean()
            feat[f"{col}_std"]  = arr.std(ddof=0)
            feat[f"{col}_min"]  = arr.min()
            feat[f"{col}_max"]  = arr.max()

        feat["label"] = label
        feat["session_id"] = session_id
        feat["source_file"] = source_file
        rows.append(feat)

    return pd.DataFrame(rows)


def main():
    sessions = load_all_sessions()
    windows_list = []
    for df in sessions:
        wdf = window_session(df, WINDOW_SIZE, STEP_SIZE)
        windows_list.append(wdf)

    full = pd.concat(windows_list, ignore_index=True)
    print("Window dataset shape:", full.shape)

    out_path = DATA_DIR / "windows_dataset.csv"
    full.to_csv(out_path, index=False)
    print(f"Saved window dataset to {out_path}")


if __name__ == "__main__":
    main()
