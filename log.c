#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "fs.h"
#include "buf.h"

// Simple logging. Each system call that might write the file system
// should be surrounded with begin_trans() and commit_trans() calls.
//
// The log holds at most one transaction at a time. Commit forces
// the log (with commit record) to disk, then installs the affected
// blocks to disk, then erases the log. begin_trans() ensures that
// only one system call can be in a transaction; others must wait.
// 
// Allowing only one transaction at a time means that the file
// system code doesn't have to worry about the possibility of
// one transaction reading a block that another one has modified,
// for example an i-node block.
//
// Read-only system calls don't need to use transactions, though
// this means that they may observe uncommitted data. I-node and
// buffer locks prevent read-only calls from seeing inconsistent data.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing sector #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged sector #s before commit.
struct logheader {
  int n;   
  //sector[i]: 第i个log data block(编号为log.start+i+1)所存储的数据，
  //本应该写入的block的id。即write操作的destination block的id
  int sector[LOGSIZE];
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int busy; // a transaction is active
  int dev;
  struct logheader lh;
};
struct log log;

static void recover_from_log(void);

void
initlog(void)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  initlock(&log.lock, "log");
  readsb(ROOTDEV, &sb);
  log.start = sb.size - sb.nlog;
  log.size = sb.nlog;
  log.dev = ROOTDEV;
  recover_from_log();
}

// Copy committed blocks from log to their home location
static void 
install_trans(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
    struct buf *dbuf = bread(log.dev, log.lh.sector[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    brelse(lbuf); 
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.sector[i] = lh->sector[i];
  }
  brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  //log变量是in-memory的，hb才是要写入磁盘的log header的表示
  //用bwrite(buf)写。(bwrite肯定就没有log transaction了--实际也是如此--
  //不然log自身的写入又要log，循环了，没法写入了...-,-)
  for (i = 0; i < log.lh.n; i++) {
    hb->sector[i] = log.lh.sector[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(void)
{
  read_head();      
  install_trans(); // if committed, copy from log to disk
  log.lh.n = 0;
  write_head(); // clear the log
}

void
begin_trans(void)
{
  acquire(&log.lock);
  while (log.busy) {
    sleep(&log, &log.lock);
  }
  log.busy = 1;
  release(&log.lock);
}

void
commit_trans(void)
{
  if (log.lh.n > 0) {
    write_head();    // Write header to disk -- the real commit
    install_trans(); // Now install writes to home locations
    log.lh.n = 0; 
    write_head();    // Erase the transaction from the log
  }
  
  acquire(&log.lock);
  log.busy = 0;
  wakeup(&log);
  release(&log.lock);
}

// Caller has modified b->data and is done with the buffer.
// Append the block to the log and record the block number, 
// but don't write the log header (which would commit the write).
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;

  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  if (!log.busy)
    panic("write outside of trans");

  //找以前有的log，因为在一个transcation中，可能对同一个block
  //多次写操作，后面的覆盖了前面的操作,这样就直接改之前的log
  //而不用append，提高效率
  //
  //同时从这里可以看出，log.lh.sector[i]存储值是buf要写入的block
  //的id，即destination addr. 而buf的数据，则存储在log section中
  //存储data的第i个block，即下面的log.start+i+1
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.sector[i] == b->sector)   // log absorbtion?
      break;
  }
  log.lh.sector[i] = b->sector;
  //sector[i]对应的block所在的block id就是log.start+i+1,
  //因为log sector也是顺序存储的
  struct buf *lbuf = bread(b->dev, log.start+i+1);
  memmove(lbuf->data, b->data, BSIZE);
  bwrite(lbuf);
  brelse(lbuf);
  if (i == log.lh.n)
    log.lh.n++;
  b->flags |= B_DIRTY; // XXX prevent eviction
}

//PAGEBREAK!
// Blank page.

