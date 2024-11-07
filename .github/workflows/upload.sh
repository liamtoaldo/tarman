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

mkdir linux
cp ./bin/tarman linux/tarman
cp -r ./bin/plugin linux/plugins
cp -r ./bin/plugin-sdk.o linux/plugin-sdk.o

mkdir tarman
cp ./bin/tarman ./tarman/tarman
tar -cvzf ./linux/tarman-linux.tar.gz ./tarman
rm -rf ./tarman

git add linux/

git commit -m "new bundle"
git push --set-upstream origin latest-build --force
