git config --local user.email "41898282+github-actions[bot]@users.noreply.github.com"
git config --local user.name "github-actions[bot]"
git checkout --orphan latest-build

git rm -rf *.md
git rm -rf .github
git rm -rf include
git rm -rf src
git rm -rf .gitattributes
git rm -rf .gitignore
git rm -rf .clang-format
git rm -rf Makefile
git rm -rf plugins
git rm -rf compile_flags.txt

mkdir $1
cp ./bin/tarman $1/tarman
cp -r ./bin/plugin $1/plugins
cp -r ./bin/plugin-sdk.o $1/plugin-sdk.o

mkdir tarman
cp ./bin/tarman ./tarman/tarman
tar -cvzf ./$1/tarman-$1.tar.gz ./tarman
rm -rf ./tarman

git add $1/

git commit -m "new bundle"
git push --set-upstream origin latest-build --force
