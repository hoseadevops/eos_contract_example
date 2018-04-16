EOS Dawn 3.0 4月5号发布了，在概念方面，相关的文章在简书有特别多介绍，例如：[【长文翻译】EOS Dawn 3.0上线，可支持百万吞吐量](https://www.jianshu.com/p/c62ff43750f2?utm_campaign=maleskine&utm_content=note&utm_medium=seo_notes&utm_source=recommendation)

这里只说说从搭建、合约方面看看：

#### 1、主要程序“更名”：
| dawn 2.0        | dawn 3.0    |
|:-------------:|:-------------:| 
| eosd      | nodeos |
| eosc     | cleos    |
| eos-wallet | keosd     |

#### 2、账户名称
账户名称，从原来的2~32个长度，改为13字符长度，同时只能使用下面32个字符
.12345abcdefghijklmnopqrstuvwxyz（即. + 1~5 + a~z）


#### 3、mongodb设置
数据写入mongodb的方式，必须添加mongo_db_plugin，*原先即使不添加plugin也是可以配置mongo_uri设置的*；

#### 4、genesis.json修改很大
producer-name只有eosio，不像之前需要设置21个生产者，//TODO 需要深入研究；

#### 5、钱包应用
创建方式等没有改变，只是创建的时候，默认将eosio的master key加入钱包；

#### 6、一些默认的合约
在2.0版本，原先可以创建链之后，就可以创建账户，初始化eos币，转账等等；3.0版本是没有，需要先上传智能合约eosio.bios：
`cleos set contract eosio build/contracts/eosio.bios -p eosio`
之后，你就可以创建账号了；
然后，就是货币问题，创建链之后，本身是没有eos币的，首先需要创建用于创建token的智能合约(有点绕，就是首先要有创建功能，然后在来创建token)，上传智能合约eosio.token(*当然要先创建该account*)：
`cleos set contract eosio.token build/contracts/eosio.token -p eosio.token`
创建token的智能合约之后，就可以，创建原先默认的EOS币了，相关方法见[eos wiki - contract](https://github.com/EOSIO/eos/wiki/Tutorial-Getting-Started-With-Contracts);
(*虽然是智能合约，但是通过get table是看不到账户的余额的，需要使用cleos get balance才能看到余额*)

#### 7、其他一些改动
 * 区块的生成速度，从3秒->0.5秒，暂时没有发现设置的地方
 * 执行智能合约action的时候，不用设置scope了
 * 智能合约传递参数，可以使用字典的方式{"name":"value"}，也可以用列表方式{"value1", "value2"}
 * 智能合约库函数、编写格式改动较大

#### 8、智能合约库函数修改
  * 合约的数据库修改，原先的字符串索引暂时删除，根据见面会的描述，该功能会在 EOS 1.0发布前添加；

