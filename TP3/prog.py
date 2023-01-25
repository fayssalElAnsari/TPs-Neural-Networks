from sre_parse import CATEGORIES
from unicodedata import numeric
from pandas.api.types import is_numeric_dtype
import pandas as pd
import os
from constantes import *
import numpy as np
from utils import *

# help: https://pbpython.com/categorical-encoding.html


def get_average(df: pd.DataFrame) -> pd.DataFrame:
    """
    `here we only take the first 100 lines in order to make the calculations fast`

    Args:
        df (dataframe): The dataframe to calculate the average of each column from

    Returns:
        dataframe: a new dataframe containing the average of each column in a new line

    """
    return df.head(100).mean(axis=0)


def get_na_percentage(df: pd.DataFrame) -> pd.DataFrame:
    """
    Args:
        df (dataframe): The dataframe to calculate the percentage of NaN
            values for each column from.

    Returns:
        dataframe: a new dataframe containing the percentage for each column,
            each percentage is in a new line. 
    """
    return df.isna().sum()/len(df)


def get_line_containing_na_percentage(df: pd.DataFrame):
    """
    Args: 
        df (dataframe): The dataframe to calculate the percentage of lines
            containing NaN from.

    Returns:
        long: the percentage of lines containing NaN in the given dataframe.
    """
    return (df.isnull().any(axis=1).sum())/len(df)


def fill_numeric_na(df: pd.DataFrame) -> pd.DataFrame:
    """
    Args (dataframe): the dataframe to fill the numeric columns of with the
        average when we have NaN.

    Returns:
        dataframe: a new filled out dataframe without NaN in numeric columns.
    """
    return df.fillna(value=get_average(df))


def get_most_frequent(df: pd.DataFrame) -> pd.DataFrame:
    """
    Args (dataframe): the dataframe to get the most frequent value of each
        column of.

    Returns:
        dataframe: a new dataframe containing the most frequent value of 
        each column. Each value is contained in a new line of the dataframe.
    """
    return df.mode().iloc[0]


def fill_with_most_frequent(df: pd.DataFrame) -> pd.DataFrame:
    """
    Args (dataframe): the dataframe to be filled with the most frequent values.

    returns:
        dataframe: a new dataframe with all NaNs filled out with the most frequent
            values.
    """
    return df.fillna(value=get_most_frequent(df))


def get_std_deviation(df: pd.DataFrame) -> pd.DataFrame:
    return df.std()# not sure?


# EX3
def encode_columns_find_replace(df: pd.DataFrame) -> pd.DataFrame:
    """
    this functions will scan all the columns of category `textuel, ensemblist and ordinal`
    then change each value to a index in the categories list

    Args (dataframe): the dataframe to be encoded.

    returns:
        dataframe: a new dataframe containing only numerical values to simplify the 
                average and variance of each column
    """
    # for each ensemblistes value we need to create a list of possible values
    # when a new value is read we need to add it to the list
    ensemblistes_values = {}

    # make a list of unique values for each column in ensemblistes_values
    for column in df:  # layer of security?
        if column in categories["ensemblistes"]:
            ensemblistes_values[column] = df[column].unique()

    # make a dictionary containing a dictionary, the inner one contains as keys
    #   the values (non duplicated) and the values are the index, to encode...
    for column in df.columns:
        if column in ensemblistes_values:
            ensemblistes_values[column] = dict(zip(ensemblistes_values[column], [
                                               i for i in range(len(ensemblistes_values[column]))]))

    # take only first 1000 rows?
    df = df.head(1000).replace(ensemblistes_values)
    return df


def encode_columns_label_encoding(df: pd.DataFrame) -> pd.DataFrame:
    df = df.head(1000)  # taking only the first 1000 rows
    for column in df.head(1000).columns:
        if column in categories["ensemblistes"]:
            df[column] = df[column].astype('category')
    df[column] = df[column].astype('category').cat.codes
    return df

def get_prix(df, k, L, nom):
    val = 0
    distances = []
    for line in range(len(df)):
        distances.append(distance(df, L, line, nom))
    lines_indices = [i[0] for i in sorted(enumerate(distances), key=lambda x:x[1])]
    for i in range(k):
        if df[nom][lines_indices[i]]:
            val += df[nom][lines_indices[i]]
    val = val/k
    return val

# EX4
def encode_columns_one_hot(df: pd.DataFrame) -> pd.DataFrame:
    # taking only first 1000 rows
    return pd.get_dummies(df.head(1000), columns=categories["ensemblistes"])


# EX5.1
def normalisation_num(df: pd.DataFrame) -> pd.DataFrame:
    df = df - df.min() / (df.max() - df.min())
    return df


# EX5.3
def standardisation(df: pd.DataFrame) -> pd.DataFrame:
    df = (df-df.mean())/df.std()
    return df

def distance(df, L1, L2, nom):
    """
     \[d(x, y) = \sqrt{\sum_{i=0}^{n}(x_{i}-y_{i})^{2}}\] 

    """
    distance = 0
    for colname in df.columns:
        if colname != nom and is_numeric_dtype(df[colname]):
            x = df[colname][L1]
            y = df[colname][L2]
            distance += x**2 + y**2
    distance = distance**(1/2)
    #print(distance) 
    #the distance between the first and the other lines is strictly ascending, is it wrong??
    return distance

def categorize_price(df):
    max_price = df['price'].max()
    print(max_price)
    bins = []
    for i in range(5):
        bins.append( (i*max_price)/5 )
    cp = pd.cut(df['price'], bins=bins, include_lowest=True, labels=list(range(1, 5)) )
    df['cp'] = cp
    return df

def predict_price(df):
    pp = []
    for i in range(len(df)//10):
        pp.append(get_prix(df, 10, i, 'price'))
    df['pp'] = pp
    return pp
    

def main():
    # columns categories dictionary
    print(categories)

    dir_path = os.path.dirname(__file__)
    file_path = os.path.join(dir_path, "db", "winemag-data-130k-v2.csv")
    mydata = pd.read_csv(file_path, na_values=["", "Na", "NaN"])

    # EX4
    filled_out_num = fill_numeric_na(mydata)
    mydata_4 = encode_columns_one_hot(filled_out_num)
    # print(mydata_4)
    print(mydata_4)
    # distance(mydata_4, 1, 2, "price")
    # print(get_prix(mydata_4, 100, 1, "price"))
    my_data_categorized = categorize_price(mydata_4)
    print(predict_price(my_data_categorized))



if __name__ == "__main__":
    main()
