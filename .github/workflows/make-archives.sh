git config --local user.email "41898282+github-actions[bot]@users.noreply.github.com"
git config --local user.name "github-actions[bot]"
git checkout --orphan latest-build

mkdir $1
cp ./bin/tarman $1/tarman
cp -r ./bin/plugins $1/plugins
cp ./bin/plugin-sdk.o $1/plugin-sdk.o

mkdir tarman
cp ./bin/tarman ./tarman/tarman
tar -cvzf ./$1/tarman-$1.tar.gz ./tarman
rm -rf ./tarman

git add $1/
