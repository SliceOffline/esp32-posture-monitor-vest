import numpy as np
import pandas as pd
from sklearn.linear_model import LogisticRegression

DATA_PATH = "windows_dataset.csv"


def main():
    df = pd.read_csv(DATA_PATH)
    print("Loaded:", df.shape, "from", DATA_PATH)

    feature_cols = [c for c in df.columns if c not in ["label", "session_id", "source_file"]]
    X = df[feature_cols].values.astype(np.float32)
    y = df["label"].values.astype(int)

    # Use whole dataset for final training (evaluation done in other scripts)
    mu = X.mean(axis=0)
    sigma = X.std(axis=0)
    sigma[sigma == 0] = 1.0

    X_norm = (X - mu) / sigma

    logreg = LogisticRegression(max_iter=1000)
    logreg.fit(X_norm, y)

    w = logreg.coef_[0]
    b = logreg.intercept_[0]

    print("Number of features:", len(feature_cols))
    print("\nFeature order (must match on ESP32):")
    for i, name in enumerate(feature_cols):
        print(f"  [{i}] {name}")

    print("\n\n// ====== Paste into model_params.cpp ======")
    print("const float LR_WEIGHTS[LR_NUM_FEATURES] = {")
    for i, val in enumerate(w):
        end = ",\n" if (i + 1) % 4 == 0 else ", "
        print(f"    {val:.8e}f{end}", end="")
    if len(w) % 4 != 0:
        print()  # newline if we didn't end with one
    print("};\n")

    print("const float LR_MEAN[LR_NUM_FEATURES] = {")
    for i, val in enumerate(mu):
        end = ",\n" if (i + 1) % 4 == 0 else ", "
        print(f"    {val:.8e}f{end}", end="")
    if len(mu) % 4 != 0:
        print()
    print("};\n")

    print("const float LR_STD[LR_NUM_FEATURES] = {")
    for i, val in enumerate(sigma):
        end = ",\n" if (i + 1) % 4 == 0 else ", "
        print(f"    {val:.8e}f{end}", end="")
    if len(sigma) % 4 != 0:
        print()
    print("};\n")

    print(f"const float LR_BIAS = {b:.8e}f;")
    print("// =========================================")


if __name__ == "__main__":
    main()
