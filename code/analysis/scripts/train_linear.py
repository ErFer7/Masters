from argparse import ArgumentParser, Namespace

import numpy as np
from model_common import ModelFactory, add_common_arguments, preprocessing_steps, run_experiment
from sklearn.compose import TransformedTargetRegressor
from sklearn.linear_model import ElasticNetCV, LassoCV, LinearRegression, RidgeCV
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler

SCRIPT_NAME = 'linear'


def make_model(regressor) -> TransformedTargetRegressor:
    return TransformedTargetRegressor(
        regressor=Pipeline([*preprocessing_steps(), ('model', regressor)]),
        transformer=StandardScaler(),
    )


def ordinary_least_squares() -> TransformedTargetRegressor:
    return make_model(LinearRegression())


def ridge() -> TransformedTargetRegressor:
    return make_model(RidgeCV(alphas=np.logspace(-3, 3, 25), cv=5))


def lasso() -> TransformedTargetRegressor:
    return make_model(LassoCV(cv=5, max_iter=200000, random_state=42))


def elastic_net() -> TransformedTargetRegressor:
    return make_model(ElasticNetCV(l1_ratio=[0.1, 0.5, 0.9], cv=5, max_iter=200000, random_state=42))  # type: ignore


def build_models() -> dict[str, ModelFactory]:
    return {
        'ols': ordinary_least_squares,
        'ridge': ridge,
        'lasso': lasso,
        'elastic_net': elastic_net,
    }


def describe_coefficients(name: str, model, feature_names: list[str]) -> str:
    estimator = model.regressor_.named_steps['model']

    lines = ['Coefficients are on standardized features and a standardized target (comparable across features).']

    if hasattr(estimator, 'alpha_'):
        lines.append(f'Selected alpha: {estimator.alpha_:.6g}')
    if hasattr(estimator, 'l1_ratio_'):
        lines.append(f'Selected l1_ratio: {estimator.l1_ratio_}')

    ranked = sorted(zip(feature_names, estimator.coef_), key=lambda item: abs(item[1]), reverse=True)
    for feature, coefficient in ranked:
        lines.append(f'  {feature:<60s} {coefficient:+.6f}')

    return '\n'.join(lines)


def main(args: Namespace) -> None:
    run_experiment(args, SCRIPT_NAME, build_models(), describe_coefficients)


if __name__ == '__main__':
    parser = ArgumentParser(description='Evaluate linear models under a hardware counter budget')
    add_common_arguments(parser)

    main(parser.parse_args())
