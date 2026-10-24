"""Bounded, on-demand grouped reads of the same validation reference branches."""
class ReferenceReader:
    def __init__(self,tree,names,events,block_size=16):
        self.tree=tree
        self.names=list(names)
        self.index={name:i for i,name in enumerate(self.names)}
        if len(self.index)!=len(self.names):raise ValueError("Duplicate reference branch")
        self.events=events
        self.block_size=block_size
        self.block=None
        self.values={}

    def read(self,name):
        block=self.index[name]//self.block_size
        if block!=self.block:
            start=block*self.block_size
            self.values=self.tree.arrays(self.names[start:start+self.block_size],entry_stop=self.events,how=dict)
            self.block=block
        return self.values[name]
