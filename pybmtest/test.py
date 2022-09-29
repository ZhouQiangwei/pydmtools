import pybmtools as pybm
import tempfile
import os
import sys
import hashlib
import numpy as np

print(pybm.numpy)
class TestRemote():
    #fname = "http://raw.githubusercontent.com/dpryan79/pybm/master/pybmTest/test.bm"
    fname=""

    def doOpen(self):
        bm = pybm.openfile(self.fname)
        assert(bm is not None)
        return bm

    def doOpenWith(self):
        with pybm.openfile(self.fname) as bm:
            print(bm.chroms())
            #assert(bm.chroms() == {'1': 195471971, '10': 130694993})

    def doChroms(self, bm):
        #assert(bm.chroms() == {'1': 195471971, '10': 130694993})
        #assert(bm.chroms("1") == 195471971)
        #assert(bm.chroms("c") is None)
        print(bm.chroms())
        print("bm.chroms")
        print(bm.chroms("chr1"))
        print("cc")
        print(bm.chroms("c"))

    def doHeader(self, bm):
        #assert(bm.header() == {'maxVal': 2, 'sumData': 272, 'minVal': 0, 'version': 4, 'sumSquared': 500, 'nLevels': 1, 'nBasesCovered': 154})
        print(bm.header())

    def doStats(self, bm):
        #type mean median max min std dev coverage cov sum
        #assert(bm.stats("1", 0, 3) == [0.2000000054637591])
        #assert(bm.stats("1", 0, 3, type="max") == [0.30000001192092896])
        #assert(bm.stats("1",99,200, type="max", nBins=2) == [1.399999976158142, 1.5])
        #assert(bm.stats("1",np.int64(99), np.int64(200), type="max", nBins=2) == [1.399999976158142, 1.5])
        #assert(bm.stats("1") == [1.3351851569281683])
        ##print(bm.stats("chr1"))
        print('bm.stats')
        #bm.stats("chr1")
        print(bm.stats("chr1", 94942, 100000))

    def doValues(self, bm):
        print("value")
        print(bm.getvalues("chr1", 9, 100000))
        #assert(bm.getvalues("1", 0, 3) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896])
        #assert(bm.getvalues("1", np.int64(0), np.int64(3)) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896])
        #assert(bm.getvalues("1", 0, 4) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896, 'nan'])

    def doIntervals(self, bm):
        print("intv")
        print(bm.intervals("chr1", 94942,94990))
        #assert(bm.intervals("1", 0, 3) == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896)))
        #assert(bm.intervals("1", np.int64(0), np.int64(3)) == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896)))
        #assert(bm.intervals("1") == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896), (100, 150, 1.399999976158142), (150, 151, 1.5)))

    def doSum(self, bm):
        print('sum')
        print(bm.stats("chr1", 100, 151000, type="mean", nBins=1))
        #print(bm.stats("chr1", 100, 151000, type="weight", nBins=1))

    def doWrite(self, bm):
        print("zzzz\n")
        #ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = "te.bm" #ofile.name
        #ofile.close()
        bm2 = pybm.openfile(oname, "w", end="Y")

        assert(bm2 is not None)
        print("ccc\n")
        print(bm.chroms("chr1"))
        print("aaa\n")
        #Since this is an unordered dict(), iterating over the items can swap the order!
        chroms = [("chr1", bm.chroms("chr1")), ("chr10", bm.chroms("chr10"))]
        print(len(bm.chroms()))
        bm2.addHeader(chroms, maxZooms=1)
        #Copy the input file
        for c in chroms:
            ints = bm.intervals(c[0])
            chroms2 = []
            starts = []
            ends = []
            values = []
            if ints:
                for entry in ints:
                    chroms2.append(c[0])
                    starts.append(entry[0])
                    ends.append(entry[1])
                    values.append(entry[2])
                #print("aa write", chroms2, starts, ends, values)
                bm2.addEntries(chroms2, starts, ends=ends, values=values)
        bm2.close()
        print("closed")
        #Ensure that the copied file has the same entries and max/min/etc.
        bm2 = pybm.openfile(oname)
        print(bm.header())
        print(bm2.header())
        print(bm.chroms())
        print(bm2.chroms())
        for c in chroms:
            ints1 = bm.intervals(c[0])
            ints2 = bm2.intervals(c[0])
            assert(ints1 == ints2)
        bm.close()
        bm2.close()
        #Clean up
        os.remove(oname)

    def doWrite2(self):
        '''
        Test all three modes of storing entries. Also test to ensure that we get error messages when doing something silly

        This is a modified version of the writing example from libBigWig
        '''
        chroms = ["chr1"]*6
        starts = [0, 100, 125, 200, 220, 230, 500, 600, 625, 700, 800, 850]
        ends = [5, 120, 126, 205, 226, 231]
        values = [0.0, 1.0, 200.0, -2.0, 150.0, 25.0, 0.0, 1.0, 200.0, -2.0, 150.0, 25.0, -5.0, -20.0, 25.0, -5.0, -20.0, 25.0]
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        bm = pybm.openfile(oname, "w", end="Y")
        print("oname", oname)
        bm.addHeader([("chr1", 1000000), ("chr2", 1500000)])
        print("add hdr correct")

        #Intervals
        bm.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
        bm.addEntries(chroms[3:6], starts[3:6], ends=ends[3:6], values=values[3:6])

        print("add 1 suc")
        #IntervalSpans, not valid in bm
        #bm.addEntries("chr1", starts[6:9], values=values[6:9], span=20)
        #bm.addEntries("chr1", starts[9:12], values=values[9:12], span=20)

        #IntervalSpanSteps, this should instead take an int
        #bm.addEntries("chr1", 900, values=values[12:15], span=20, step=30)
        #bm.addEntries("chr1", 990, values=values[15:18], span=20, step=30)

        print("add suscs")

        #Attempt to add incorrect values. These MUST raise an exception
        try:
            bm.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
            assert(1==0)
        except RuntimeError:
            pass
        #try:
        #    bm.addEntries("chr1", starts[6:9], values=values[6:9], span=20)
        #    assert(1==0)
        #except RuntimeError:
        #    pass
        #try:
        #    bm.addEntries("chrxx", starts[6:9], values=values[6:9], span=20)
        #    assert(1==0)
        #except RuntimeError:
        #    pass
        #try:
        #    bm.addEntries("chr1", 900, values=values[12:15], span=20, step=30)
        #    assert(1==0)
        #except RuntimeError:
        #    pass

        #Add a few intervals on a new chromosome
        bm.addEntries(["chr2"]*3, starts[0:3], ends=ends[0:3], values=values[0:3])
        bm.close()
        #check md5sum, this is the simplest method to check correctness
        #h = hashlib.md5(open(oname, "rb").read()).hexdigest()
        #assert(h=="ef104f198c6ce8310acc149d0377fc16")
        #Clean up
        os.remove(oname)

    def doWriteEmpty(self):
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        bm = pybm.openfile(oname, "w", end="Y")
        bm.addHeader([("1", 1000000), ("2", 1500000)])
        bm.close()

        #check md5sum
        h = hashlib.md5(open(oname, "rb").read()).hexdigest()
        print(h)
        #assert(h=="361c600e5badf0b45d819552a7822937")

        #Ensure we can openfile and get reasonable results
        bm = pybm.openfile(oname)
        assert(bm.chroms() == {'1': 1000000, '2': 1500000})
        assert(bm.intervals("1") == None)
        assert(bm.getvalues("1", 0, 1000000) == [])
        assert(bm.stats("1", 0, 1000000, nBins=2) == [None, None])
        bm.close()

        #Clean up
        os.remove(oname)

    def doWriteNumpy(self):
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        bm = pybm.openfile(oname, "w", end="Y")
        bm.addHeader([("chr1", 100), ("chr2", 150), ("chr3", 200), ("chr4", 250)])
        chroms = np.array(["chr1"] * 2 + ["chr2"] * 2 + ["chr3"] * 2 + ["chr4"] * 2)
        starts = np.array([0, 10, 40, 50, 60, 70, 80, 90], dtype=np.int64)
        ends = np.array([5, 15, 45, 55, 65, 75, 85, 95], dtype=np.int64)
        values0 = np.array(np.random.random_sample(8), dtype=np.float64)
        bm.addEntries(chroms, starts, ends=ends, values=values0)
        bm.close()

        vals = [(x, y, z) for x, y, z in zip(starts, ends, values0)]
        bm = pybm.openfile(oname)
        assert(bm.chroms() == {'chr1': 100, 'chr2': 150, 'chr3': 200, 'chr4': 250})
        for idx1, chrom in enumerate(["chr1", "chr2", "chr3", "chr4"]):
            for idx2, tup in enumerate(bm.intervals(chrom)):
                assert(tup[0] == starts[2 * idx1 + idx2])
                assert(tup[1] == ends[2 * idx1 + idx2])
                assert(np.isclose(tup[2], values0[2 * idx1 + idx2]))
        bm.close()

        #Clean up
        os.remove(oname)

    def testAll(self):
        print("test all")
        bm = self.doOpen()
        self.doChroms(bm)
        if not self.fname.startswith("http"):
            self.doHeader(bm)
            self.doStats(bm)
            self.doSum(bm)
            self.doValues(bm)
            self.doIntervals(bm)
            self.doWrite(bm)
            self.doOpenWith()
            print("self.doOpenWith")
            self.doWrite2()
            print("self.doWrite2")
            self.doWriteEmpty()
            print("doWriteEmpty")
            self.doWriteNumpy()
        bm.close()

class TestLocal():
    def testFoo(self):
        blah = TestRemote()
        blah.fname = "pybmtest/test.bm"
        blah.testAll()

print("xxxx")
tl=TestLocal()
tl.testFoo()

class TestNumpy():
    def testNumpy(self):
        import os
        if pybm.numpy == 0:
            return 0
        import numpy as np

        bm = pybm.openfile("pybmtest/testnp.bm", "w")
        bm.addHeader([("chr1", 1000)], maxZooms=0)
        # Type 0
        chroms = np.array(["chr1"] * 10)
        starts = np.array([0, 10, 20, 30, 40, 50, 60, 70, 80, 90], dtype=np.int64)
        ends = np.array([5, 15, 25, 35, 45, 55, 65, 75, 85, 95], dtype=np.int64)
        values0 = np.array(np.random.random_sample(10), dtype=np.float64)
        bm.addEntries(chroms, starts, ends=ends, values=values0)

        starts = np.array([100, 110, 120, 130, 140, 150, 160, 170, 180, 190], dtype=np.int64)
        ends = np.array([105, 115, 125, 135, 145, 155, 165, 175, 185, 195], dtype=np.int64)
        values1 = np.array(np.random.random_sample(10), dtype=np.float64)
        bm.addEntries(chroms, starts, ends=ends, values=values1)

        # Type 1, single chrom, multiple starts/values, single span
        #starts = np.array([200, 210, 220, 230, 240, 250, 260, 270, 280, 290], dtype=np.int64)
        #values2 = np.array(np.random.random_sample(10), dtype=np.float64)
        #bm.addEntries(np.str("chr1"), starts, span=np.int(8), values=values2)

        #starts = np.array([300, 310, 320, 330, 340, 350, 360, 370, 380, 390], dtype=np.int64)
        #values3 = np.array(np.random.random_sample(10), dtype=np.float64)
        #bm.addEntries(np.str("chr1"), starts, span=np.int(8), values=values3)

        # Type 2, single chrom/start/span/step, multiple values
        #values4 = np.array(np.random.random_sample(10), dtype=np.float64)
        #bm.addEntries(np.str("chr1"), np.int(400), span=np.int(8), step=np.int64(2), values=values4)

        #values5 = np.array(np.random.random_sample(10), dtype=np.float64)
        #bm.addEntries(np.str("chr1"), np.int(500), span=np.int(8), step=np.int64(2), values=values5)

        bm.close()

        bm = pybm.openfile("pybmtest/testnp.bm")
        assert(bm is not None)

        def compy(start, v2):
            v = []
            for t in bm.intervals("chr1", start, start + 100):
                v.append(t[2])
            v = np.array(v)
            assert(np.all(np.abs(v - v2) < 1e-5))

        compy(0, values0)
        compy(100, values1)
        #compy(200, values2)
        #compy(300, values3)
        #compy(400, values4)
        #compy(500, values5)

        # Get values as a numpy array
        foo = bm.getvalues("chr1", 0, 100, numpy=False)
        assert(isinstance(foo, list))
        foo = bm.getvalues("chr1", 0, 100, numpy=True)
        assert(isinstance(foo, np.ndarray))

        bm.close()
        #os.remove("test/outnp.bm")

    def testNumpyValues(self):
        if pybm.numpy == 0:
            return 0
        import numpy as np

        fname = "pybmtest/testnp.bm"
        bm = pybm.openfile(fname, "r")

        print(bm.getvalues("chr1", 0, 100, numpy=True))
        assert np.allclose(
            bm.getvalues("chr1", 0, 100, numpy=True),
            np.array(bm.getvalues("chr1", 0, 100), dtype=np.float32),
            equal_nan=True
        )

        assert np.allclose(
            bm.stats("chr1", 0, 20, "mean", 5, numpy=True),
            np.array(bm.stats("chr1", 0, 20, "mean", 5), dtype=np.float64),
            equal_nan=True
        )

tn=TestNumpy()
tn.testNumpy()
tn.testNumpyValues()
