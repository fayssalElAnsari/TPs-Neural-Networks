## TP 3 : Pre-traitement (Pre-processing)

### Le probl`eme des donn´ees manquantes (suite et fin)
1. here we'are asked to redo the same steps of the previous exercise paper untill exercise 4. So what were these steps?
   1. Open the file in a `DataFrame` format using `Pandas`.
        * `pd.readcsv("filename", "NA")`
   2. Apply the one hot encoding technique on the `ensemblistes` colomns of this db.
        * `pd.get_dummies(df.head(1000), columns=categories["ensemblistes"])`
2. Now we need to restore the original values of the `price` column.
3. How to calculate the distance between two lines? What does `distance` mean here?
    * the expression of distance is given as:

 $$ d(x, y) = \sqrt{\sum_{i=0}^{n}(x_{i}-y_{i})^{2}} $$
 where `n` is the number of columns of which we can calcualte the distance, we can only calculate the distance for columns that have been encoded. In the case of one hot encoding we will have many columns (our case) for each `ensembliste` column.

4. forgot to call replace....
5. how to add a column to a dataframe?
   