import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.svm import SVC
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix

DATA_PATH = "windows_dataset.csv"


def main():
    df = pd.read_csv(DATA_PATH)
    print("Loaded:", df.shape, "from", DATA_PATH)

    feature_cols = [c for c in df.columns if c not in ["label", "session_id", "source_file"]]
    X = df[feature_cols].values.astype(np.float32)
    y = df["label"].values.astype(int)

    X_train, X_test, y_train, y_test = train_test_split(
        X, y,
        test_size=0.2,
        random_state=42,
        stratify=y
    )

    # Normalize using TRAIN only
    mu = X_train.mean(axis=0)
    sigma = X_train.std(axis=0)
    sigma[sigma == 0] = 1.0

    X_train_n = (X_train - mu) / sigma
    X_test_n = (X_test - mu) / sigma

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

    print("\n=== Random-split model comparison ===")
    for name, clf in models.items():
        clf.fit(X_train_n, y_train)
        y_pred = clf.predict(X_test_n)
        acc = accuracy_score(y_test, y_pred)
        print(f"{name:18s}  accuracy = {acc:.3f}")

    # Detailed report for logistic regression
    logreg = models["LogisticRegression"]
    y_pred = logreg.predict(X_test_n)

    print("\n=== Logistic Regression detailed report ===")
    print("Accuracy:", accuracy_score(y_test, y_pred))
    print("Confusion matrix:\n", confusion_matrix(y_test, y_pred))
    print("\nClassification report:\n", classification_report(y_test, y_pred))


if __name__ == "__main__":
    main()
