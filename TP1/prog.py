import pandas

def main():
    mydata = pandas.read_csv('./db/winemag-data_first150k.csv')
    print(mydata.head())
    print(mydata.columns)

if __name__ == "__main__":
    main()