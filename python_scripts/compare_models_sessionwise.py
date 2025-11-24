import numpy as np
import pandas as pd
from sklearn.model_selection import GroupShuffleSplit
from sklearn.linear_model import LogisticRegression
from sklearn.svm import SVC
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, confusion_matrix, classification_report

DATA_PATH = "windows_dataset.csv"


def main():
    df = pd.read_csv(DATA_PATH)
    print("Loaded:", df.shape, "from", DATA_PATH)

    feature_cols = [c for c in df.columns if c not in ["label", "session_id", "source_file"]]
    X_all = df[feature_cols].values.astype(np.float32)
    y_all = df["label"].values.astype(int)
    groups = df["session_id"].values

    models = {
        "LogisticRegression": LogisticRegression(max_iter=1000),
        "LinearSVM": SVC(kernel="linear", probability=False),
        "RandomForest": RandomForestClassifier(
            n_estimators=100,
            max_depth=None,
            random_state=42,
        ),
        "MLP_16": MLPClassifier(
            hidden_layer_sizes=(16,),
            activation="relu",
            max_iter=500,
            random_state=42,
        ),
    }

    gss = GroupShuffleSplit(n_splits=5, test_size=0.2, random_state=42)
    accs = {name: [] for name in models.keys()}

    print("\n=== Session-wise model comparison (GroupShuffleSplit) ===")
    for fold, (train_idx, test_idx) in enumerate(gss.split(X_all, y_all, groups)):
        X_tr, X_te = X_all[train_idx], X_all[test_idx]
        y_tr, y_te = y_all[train_idx], y_all[test_idx]

        mu = X_tr.mean(axis=0)
        sigma = X_tr.std(axis=0)
        sigma[sigma == 0] = 1.0

        X_tr_n = (X_tr - mu) / sigma
        X_te_n = (X_te - mu) / sigma

        print(f"\nFold {fold}:")
        lr_y_pred = None  # will store LR predictions for detailed report

        for name, clf in models.items():
            clf.fit(X_tr_n, y_tr)
            y_pred = clf.predict(X_te_n)
            acc = accuracy_score(y_te, y_pred)
            accs[name].append(acc)
            print(f"  {name:18s} accuracy = {acc:.3f}")

            if name == "LogisticRegression":
                lr_y_pred = y_pred  # save for detailed metrics

        # Detailed report for Logistic Regression on this fold
        if lr_y_pred is not None:
            print("\n  Logistic Regression detailed report (this fold):")
            print("  Confusion matrix:")
            cm = confusion_matrix(y_te, lr_y_pred)
            print("  ", cm[0])
            print("  ", cm[1])
            print("\n  Classification report:")
            cr = classification_report(y_te, lr_y_pred, zero_division=0)
            print(cr)

    print("\n=== Mean session-wise accuracies over folds ===")
    for name, vals in accs.items():
        vals = np.array(vals)
        print(f"{name:18s}: mean = {vals.mean():.3f}, std = {vals.std():.3f}")


if __name__ == "__main__":
    main()
