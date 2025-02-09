sudo ndctl disable-namespace all
sudo ndctl destroy-namespace all --force
#/dev/dax1.0 for SlotFS
sudo ndctl create-namespace -m devdax
#/dev/pmem0 for others
sudo ndctl create-namespace -m fsdax
ndctl list --region=0 --namespaces --human --idle

