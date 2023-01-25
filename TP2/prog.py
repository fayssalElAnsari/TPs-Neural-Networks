from sre_parse import CATEGORIES
from unicodedata import numeric
import pandas as pd
import os
from constantes import *
import numpy as np

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
        df (dataframe): The dataframe to calculate the percentage of Nan
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
    return df.std()


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

    ## make a list of unique values for each column in ensemblistes_values
    for column in df: # layer of security?
        if column in categories["ensemblistes"]:
            ensemblistes_values[column] = df[column].unique()

    ## make a dictionary containing a dictionary, the inner one contains as keys
    #   the values (non duplicated) and the values are the index, to encode...
    for column in df.columns:
        if column in ensemblistes_values:
            ensemblistes_values[column] = dict(zip(ensemblistes_values[column], [i for i in range(len(ensemblistes_values[column]))]))

    df = df.head(1000).replace(ensemblistes_values) # take only first 1000 rows?
    return df


def encode_columns_label_encoding(df: pd.DataFrame) -> pd.DataFrame:
    df = df.head(1000) #taking only the first 1000 rows
    for column in df.head(1000).columns:
        if column in categories["ensemblistes"]:
            df[column] = df[column].astype('category')
    df[column] = df[column].astype('category').cat.codes
    return df
    

# EX4
def encode_columns_one_hot(df: pd.DataFrame) -> pd.DataFrame:
    return pd.get_dummies(df.head(1000), columns=categories["ensemblistes"]) # taking only first 1000 rows


# EX5.1
def normalisation_num(df: pd.DataFrame) -> pd.DataFrame:
    df = df - df.min() / (df.max() - df.min())
    return df


# EX5.3
def standardisation(df: pd.DataFrame) -> pd.DataFrame:
    df = (df-df.mean())/df.std()    
    return df


def main():
    # columns categories dictionary
    print(categories)
    
    dir_path = os.path.dirname(__file__)
    file_path = os.path.join(dir_path, "db", "winemag-data_first150k.csv")
    mydata = pd.read_csv(file_path, na_values=["", "Na", "NaN"])

    # print(get_na_percentage(mydata))
    # filled_out_num = fill_numeric_na(mydata)
    # print(get_na_percentage(filled_out_num))
    # filled_out = fill_with_most_frequent(filled_out_num)
    # print(filled_out)
    # print(get_na_percentage(filled_out))

    # EX3
    print(get_average(mydata))
    mydata_3 = encode_columns_find_replace(mydata)
    print(fill_numeric_na(mydata_3))

    # EX4
    mydata = encode_columns_one_hot(mydata)
    print(mydata)

    # EX5
    # mydata  = normalisation_num(mydata)
    mydata = standardisation(mydata)
    print(get_average(mydata_3)) #ex3
    print(get_std_deviation(mydata_3))#ex3
    print(get_average(mydata))
    print(get_std_deviation(mydata))
    
    
if __name__ == "__main__":
    main()